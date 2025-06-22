// SPDX-License-Identifier: GPL-2.0
/*
 * This is the Fusion MPT base driver providing common API layer interface
 * for access to MPT (Message Passing Technology) firmware.
 *
 * Copyright (C) 2013-2021  LSI Corporation
 * Copyright (C) 2013-2021  Avago Technologies
 * Copyright (C) 2013-2021  Broadcom Inc.
 *  (mailto:MPT-FusionLinux.pdl@broadcom.com)
 *
 * Copyright (C) 2024 LeapIO Tech Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * NO WARRANTY
 * THE PROGRAM IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT
 * LIMITATION, ANY WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Each Recipient is
 * solely responsible for determining the appropriateness of using and
 * distributing the Program and assumes all risks associated with its
 * exercise of rights under this Agreement, including but not limited to
 * the risks and costs of program errors, damage to or loss of data,
 * programs or equipment, and unavailability or interruption of operations.

 * DISCLAIMER OF LIABILITY
 * NEITHER RECIPIENT NOR ANY CONTRIBUTORS SHALL HAVE ANY LIABILITY FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING WITHOUT LIMITATION LOST PROFITS), HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OR DISTRIBUTION OF THE PROGRAM OR THE EXERCISE OF ANY RIGHTS GRANTED
 * HEREUNDER, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/kthread.h>
#include <asm/page.h>
#include <linux/aer.h>
#include "leapioraid_func.h"
#include <linux/net.h>
#include <net/sock.h>
#include <linux/inet.h>

static char *dest_ip = "127.0.0.1";
module_param(dest_ip, charp, 0000);
MODULE_PARM_DESC(dest_ip, "Destination IP address");

static u16 port_no = 6666;
module_param(port_no, ushort, 0000);
MODULE_PARM_DESC(port_no, "Destination Port number");
static struct sockaddr_in dest_addr;
static struct socket *sock;
static struct msghdr msg;

#define LEAPIORAID_LOG_POLLING_INTERVAL 1
static LEAPIORAID_CALLBACK leapioraid_callbacks[LEAPIORAID_MAX_CALLBACKS];
#define LEAPIORAID_FAULT_POLLING_INTERVAL 1000
#define LEAPIORAID_MAX_HBA_QUEUE_DEPTH	1024

static int smp_affinity_enable = 1;
module_param(smp_affinity_enable, int, 0444);
MODULE_PARM_DESC(smp_affinity_enable,
		 "SMP affinity feature enable/disable Default: enable(1)");

static int max_msix_vectors = -1;
module_param(max_msix_vectors, int, 0444);
MODULE_PARM_DESC(max_msix_vectors, " max msix vectors");

static int irqpoll_weight = -1;
module_param(irqpoll_weight, int, 0444);
MODULE_PARM_DESC(irqpoll_weight,
		 "irq poll weight (default= one fourth of HBA queue depth)");

static int leapioraid_fwfault_debug;

static int perf_mode = -1;

static int poll_queues;
module_param(poll_queues, int, 0444);
MODULE_PARM_DESC(poll_queues,
		 "Number of queues to be use for io_uring poll mode.\n\t\t"
		 "This parameter is effective only if host_tagset_enable=1. &\n\t\t"
		 "when poll_queues are enabled then &\n\t\t"
		 "perf_mode is set to latency mode. &\n\t\t");

enum leapioraid_perf_mode {
	LEAPIORAID_PERF_MODE_DEFAULT = -1,
	LEAPIORAID_PERF_MODE_BALANCED = 0,
	LEAPIORAID_PERF_MODE_IOPS = 1,
	LEAPIORAID_PERF_MODE_LATENCY = 2,
};

static void
leapioraid_base_clear_outstanding_leapioraid_commands(
	struct LEAPIORAID_ADAPTER *ioc);
static
int leapioraid_base_wait_on_iocstate(struct LEAPIORAID_ADAPTER *ioc,
					    u32 ioc_state, int timeout);

static int
leapioraid_scsihost_set_fwfault_debug(
	const char *val, const struct kernel_param *kp)
{
	int ret = param_set_int(val, kp);
	struct LEAPIORAID_ADAPTER *ioc;

	if (ret)
		return ret;
	pr_info("setting fwfault_debug(%d)\n",
	       leapioraid_fwfault_debug);
	spin_lock(&leapioraid_gioc_lock);
	list_for_each_entry(ioc, &leapioraid_ioc_list, list)
		ioc->fwfault_debug = leapioraid_fwfault_debug;
	spin_unlock(&leapioraid_gioc_lock);
	return 0;
}

module_param_call(
	leapioraid_fwfault_debug,
	leapioraid_scsihost_set_fwfault_debug,
	param_get_int, &leapioraid_fwfault_debug, 0644);

static inline u32
leapioraid_base_readl_aero(
	const void __iomem *addr, u8 retry_count)
{
	u32 i = 0, ret_val;

	do {
		ret_val = readl(addr);
		i++;
	} while (ret_val == 0 && i < retry_count);
	return ret_val;
}

u8
leapioraid_base_check_cmd_timeout(
	struct LEAPIORAID_ADAPTER *ioc,
	U8 status, void *mpi_request, int sz)
{
	u8 issue_reset = 0;

	if (!(status & LEAPIORAID_CMD_RESET))
		issue_reset = 1;
	pr_err("%s Command %s\n", ioc->name,
	       ((issue_reset ==
		 0) ? "terminated due to Host Reset" : "Timeout"));
	leapioraid_debug_dump_mf(mpi_request, sz);
	return issue_reset;
}

static int
leapioraid_remove_dead_ioc_func(void *arg)
{
	struct LEAPIORAID_ADAPTER *ioc = (struct LEAPIORAID_ADAPTER *)arg;
	struct pci_dev *pdev;

	if (ioc == NULL)
		return -1;
	pdev = ioc->pdev;
	if (pdev == NULL)
		return -1;
#if defined(DISABLE_RESET_SUPPORT)
	ssleep(2);
#endif

	pci_stop_and_remove_bus_device(pdev);
	return 0;
}

u8
leapioraid_base_pci_device_is_unplugged(struct LEAPIORAID_ADAPTER *ioc)
{
	struct pci_dev *pdev = ioc->pdev;
	struct pci_bus *bus = pdev->bus;
	int devfn = pdev->devfn;
	u32 vendor_id;

	if (pci_bus_read_config_dword(bus, devfn, PCI_VENDOR_ID, &vendor_id))
		return 1;
	if (vendor_id == 0xffffffff || vendor_id == 0x00000000 ||
	    vendor_id == 0x0000ffff || vendor_id == 0xffff0000)
		return 1;
	if ((vendor_id & 0xffff) == 0x0001)
		return 1;
	return 0;
}

u8
leapioraid_base_pci_device_is_available(struct LEAPIORAID_ADAPTER *ioc)
{
	if (ioc->pci_error_recovery
	    || leapioraid_base_pci_device_is_unplugged(ioc))
		return 0;
	return 1;
}

static void
leapioraid_base_sync_drv_fw_timestamp(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidIoUnitControlReq_t *mpi_request;
	struct LeapioraidIoUnitControlRep_t *mpi_reply;
	u16 smid;
	ktime_t current_time;
	u64 TimeStamp = 0;
	u8 issue_reset = 0;

	mutex_lock(&ioc->scsih_cmds.mutex);
	if (ioc->scsih_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_err("%s: scsih_cmd in use %s\n", ioc->name, __func__);
		goto out;
	}
	ioc->scsih_cmds.status = LEAPIORAID_CMD_PENDING;
	smid = leapioraid_base_get_smid(ioc, ioc->scsih_cb_idx);
	if (!smid) {
		pr_err("%s: failed obtaining a smid %s\n", ioc->name, __func__);
		ioc->scsih_cmds.status = LEAPIORAID_CMD_NOT_USED;
		goto out;
	}
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->scsih_cmds.smid = smid;
	memset(mpi_request, 0, sizeof(struct LeapioraidIoUnitControlReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_IO_UNIT_CONTROL;
	mpi_request->Operation = 0x0F;
	mpi_request->IOCParameter = 0x81;
	current_time = ktime_get_real();
	TimeStamp = ktime_to_ms(current_time);
	mpi_request->IOCParameterValue = cpu_to_le32(TimeStamp & 0xFFFFFFFF);
	mpi_request->IOCParameterValue2 = cpu_to_le32(TimeStamp >> 32);
	init_completion(&ioc->scsih_cmds.done);
	ioc->put_smid_default(ioc, smid);
	dinitprintk(ioc, pr_err(
			"%s Io Unit Control Sync TimeStamp (sending), @time %lld ms\n",
			ioc->name, TimeStamp));
	wait_for_completion_timeout(&ioc->scsih_cmds.done,
				    10 * HZ);
	if (!(ioc->scsih_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		leapioraid_check_cmd_timeout(ioc,
					     ioc->scsih_cmds.status,
					     mpi_request,
					     sizeof
					     (struct LeapioraidSasIoUnitControlReq_t)
					     / 4, issue_reset);
		goto issue_host_reset;
	}
	if (ioc->scsih_cmds.status & LEAPIORAID_CMD_REPLY_VALID) {
		mpi_reply = ioc->scsih_cmds.reply;
		dinitprintk(ioc, pr_err(
			"%s Io Unit Control sync timestamp (complete): ioc_status(0x%04x), loginfo(0x%08x)\n",
			ioc->name,
			le16_to_cpu(mpi_reply->IOCStatus),
			le32_to_cpu(mpi_reply->IOCLogInfo)));
	}
issue_host_reset:
	if (issue_reset)
		leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
	ioc->scsih_cmds.status = LEAPIORAID_CMD_NOT_USED;
out:
	mutex_unlock(&ioc->scsih_cmds.mutex);
}

static int
leapioraid_udp_init(void)
{
	int ret;
	u32 ip;

	if (sock)
		return 0;
	if (!in4_pton(dest_ip, -1, (u8 *) &ip, -1, NULL)) {
		pr_err("Invalid IP address: %s, set to default: 127.0.0.1\n",
		       dest_ip);
		dest_ip = "127.0.0.1";
	}
	ret =
	    sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP,
			     &sock);
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = ip;
	dest_addr.sin_port = htons(port_no);
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &dest_addr;
	msg.msg_namelen = sizeof(struct sockaddr_in);
	return ret;
}

static void
leapioraid_udp_exit(void)
{
	if (sock)
		sock_release(sock);
}

struct info
{
	u32 user_position;
	u32 ioc_position;
};
int cooo;

static void
leapioraid_base_pcie_log_work(struct work_struct *work)
{
	struct LEAPIORAID_ADAPTER *ioc =
		container_of(work,
			struct LEAPIORAID_ADAPTER, pcie_log_work.work);
	unsigned long flags;
	struct info *infom = (struct info *)(ioc->log_buffer + SYS_LOG_BUF_SIZE);

	if (cooo == 0) {
		infom->user_position =
			 ioc->base_readl(&ioc->chip->HostLogBufPosition, 0);
		infom->ioc_position =
			 ioc->base_readl(&ioc->chip->IocLogBufPosition, 0);
		cooo++;
	}

	writel(infom->user_position, &ioc->chip->HostLogBufPosition);
	infom->ioc_position = ioc->base_readl(&ioc->chip->IocLogBufPosition, 0);

	spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock, flags);
	if (ioc->pcie_log_work_q)
		queue_delayed_work(ioc->pcie_log_work_q,
				&ioc->pcie_log_work,
				msecs_to_jiffies(LEAPIORAID_LOG_POLLING_INTERVAL));
	spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
}

void
leapioraid_base_start_log_watchdog(struct LEAPIORAID_ADAPTER *ioc)
{
	unsigned long flags;

	if (ioc->pcie_log_work_q)
		return;
	leapioraid_udp_init();
	INIT_DELAYED_WORK(&ioc->pcie_log_work, leapioraid_base_pcie_log_work);
	snprintf(ioc->pcie_log_work_q_name,
		 sizeof(ioc->pcie_log_work_q_name), "poll_%s%u_status",
		 ioc->driver_name, ioc->id);
	ioc->pcie_log_work_q =
	    create_singlethread_workqueue(ioc->pcie_log_work_q_name);
	if (!ioc->pcie_log_work_q) {
		pr_err("%s %s: failed (line=%d)\n", ioc->name,
		       __func__, __LINE__);
		return;
	}
	spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock, flags);
	if (ioc->pcie_log_work_q)
		queue_delayed_work(ioc->pcie_log_work_q,
				   &ioc->pcie_log_work,
				   msecs_to_jiffies(LEAPIORAID_LOG_POLLING_INTERVAL));
	spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
}

void
leapioraid_base_stop_log_watchdog(struct LEAPIORAID_ADAPTER *ioc)
{
	unsigned long flags;
	struct workqueue_struct *wq;

	spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock, flags);
	wq = ioc->pcie_log_work_q;
	ioc->pcie_log_work_q = NULL;
	leapioraid_udp_exit();
	spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
	if (wq) {
		if (!cancel_delayed_work_sync(&ioc->pcie_log_work))
			flush_workqueue(wq);
		destroy_workqueue(wq);
	}
}

static void
leapioraid_base_fault_reset_work(struct work_struct *work)
{
	struct LEAPIORAID_ADAPTER *ioc =
	    container_of(work, struct LEAPIORAID_ADAPTER,
			 fault_reset_work.work);
	unsigned long flags;
	u32 doorbell;
	int rc;
	struct task_struct *p;

	spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock, flags);
	if ((ioc->shost_recovery && (ioc->ioc_coredump_loop == 0)) ||
	    ioc->pci_error_recovery || ioc->remove_host)
		goto rearm_timer;
	spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
	doorbell = leapioraid_base_get_iocstate(ioc, 0);
	if ((doorbell & LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_MASK) {
		pr_err(
		       "%s SAS host is non-operational !!!!\n", ioc->name);
		if (ioc->non_operational_loop++ < 5) {
			spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock,
					  flags);
			goto rearm_timer;
		}
		ioc->remove_host = 1;
		leapioraid_base_pause_mq_polling(ioc);
		ioc->schedule_dead_ioc_flush_running_cmds(ioc);
		p = kthread_run(leapioraid_remove_dead_ioc_func, ioc,
				"%s_dead_ioc_%d", ioc->driver_name, ioc->id);
		if (IS_ERR(p))
			pr_err(
				"%s %s: Running leapioraid_dead_ioc thread failed !!!!\n",
				ioc->name, __func__);
		else
			pr_err(
				"%s %s: Running leapioraid_dead_ioc thread success !!!!\n",
				ioc->name, __func__);
		return;
	}
	if ((doorbell & LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_COREDUMP) {
		u8 timeout = (ioc->manu_pg11.CoreDumpTOSec) ?
		    ioc->manu_pg11.CoreDumpTOSec :
		    15;
		timeout /= (LEAPIORAID_FAULT_POLLING_INTERVAL / 1000);
		if (ioc->ioc_coredump_loop == 0) {
			leapioraid_base_coredump_info(ioc, doorbell &
						      LEAPIORAID_DOORBELL_DATA_MASK);
			spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock,
					  flags);
			ioc->shost_recovery = 1;
			spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock,
					       flags);
			leapioraid_base_pause_mq_polling(ioc);
			leapioraid_scsihost_clear_outstanding_scsi_tm_commands
			    (ioc);
			leapioraid_base_mask_interrupts(ioc);
			leapioraid_base_clear_outstanding_leapioraid_commands(ioc);
			leapioraid_ctl_clear_outstanding_ioctls(ioc);
		}
		drsprintk(ioc,
			  pr_info("%s %s: CoreDump loop %d.",
				 ioc->name, __func__, ioc->ioc_coredump_loop));
		if (ioc->ioc_coredump_loop++ < timeout) {
			spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock,
					  flags);
			goto rearm_timer;
		}
	}
	if (ioc->ioc_coredump_loop) {
		if ((doorbell & LEAPIORAID_IOC_STATE_MASK) !=
		    LEAPIORAID_IOC_STATE_COREDUMP)
			pr_err(
			       "%s %s: CoreDump completed. LoopCount: %d",
			       ioc->name, __func__, ioc->ioc_coredump_loop);
		else
			pr_err(
			       "%s %s: CoreDump Timed out. LoopCount: %d",
			       ioc->name, __func__, ioc->ioc_coredump_loop);
		ioc->ioc_coredump_loop = 0xFF;
	}
	ioc->non_operational_loop = 0;
	if ((doorbell & LEAPIORAID_IOC_STATE_MASK) !=
	    LEAPIORAID_IOC_STATE_OPERATIONAL) {
		rc = leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
		pr_warn("%s %s: hard reset: %s\n", ioc->name,
		       __func__, (rc == 0) ? "success" : "failed");
		doorbell = leapioraid_base_get_iocstate(ioc, 0);
		if ((doorbell & LEAPIORAID_IOC_STATE_MASK) ==
		    LEAPIORAID_IOC_STATE_FAULT) {
			leapioraid_print_fault_code(ioc,
						    doorbell &
						    LEAPIORAID_DOORBELL_DATA_MASK);
		} else if ((doorbell & LEAPIORAID_IOC_STATE_MASK) ==
			   LEAPIORAID_IOC_STATE_COREDUMP)
			leapioraid_base_coredump_info(ioc,
						      doorbell &
						      LEAPIORAID_DOORBELL_DATA_MASK);
		if (rc
		    && (doorbell & LEAPIORAID_IOC_STATE_MASK) !=
		    LEAPIORAID_IOC_STATE_OPERATIONAL)
			return;
	}
	ioc->ioc_coredump_loop = 0;
	if (ioc->time_sync_interval &&
	    ++ioc->timestamp_update_count >= ioc->time_sync_interval) {
		ioc->timestamp_update_count = 0;
		leapioraid_base_sync_drv_fw_timestamp(ioc);
	}
	spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock, flags);
rearm_timer:
	if (ioc->fault_reset_work_q)
		queue_delayed_work(ioc->fault_reset_work_q,
				   &ioc->fault_reset_work,
				   msecs_to_jiffies(LEAPIORAID_FAULT_POLLING_INTERVAL));
	spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
}

static void
leapioraid_base_hba_hot_unplug_work(struct work_struct *work)
{
	struct LEAPIORAID_ADAPTER *ioc =
	    container_of(work, struct LEAPIORAID_ADAPTER,
			 hba_hot_unplug_work.work);
	unsigned long flags;

	spin_lock_irqsave(&ioc->hba_hot_unplug_lock, flags);
	if (ioc->shost_recovery || ioc->pci_error_recovery)
		goto rearm_timer;
	if (leapioraid_base_pci_device_is_unplugged(ioc)) {
		if (ioc->remove_host) {
			pr_err("%s The host is removeing!!!\n",
				ioc->name);
			goto rearm_timer;
		}
		ioc->remove_host = 1;
		leapioraid_base_clear_outstanding_leapioraid_commands(ioc);
		leapioraid_base_pause_mq_polling(ioc);
		leapioraid_scsihost_clear_outstanding_scsi_tm_commands(ioc);
		leapioraid_ctl_clear_outstanding_ioctls(ioc);
	}
rearm_timer:
	if (ioc->hba_hot_unplug_work_q)
		queue_delayed_work(ioc->hba_hot_unplug_work_q,
				   &ioc->hba_hot_unplug_work,
				   msecs_to_jiffies
				   (1000));
	spin_unlock_irqrestore(&ioc->hba_hot_unplug_lock, flags);
}

void
leapioraid_base_start_watchdog(struct LEAPIORAID_ADAPTER *ioc)
{
	unsigned long flags;

	if (ioc->fault_reset_work_q)
		return;
	ioc->timestamp_update_count = 0;
	INIT_DELAYED_WORK(&ioc->fault_reset_work,
		leapioraid_base_fault_reset_work);
	snprintf(ioc->fault_reset_work_q_name,
		 sizeof(ioc->fault_reset_work_q_name), "poll_%s%u_status",
		 ioc->driver_name, ioc->id);
	ioc->fault_reset_work_q =
	    create_singlethread_workqueue(ioc->fault_reset_work_q_name);
	if (!ioc->fault_reset_work_q) {
		pr_err("%s %s: failed (line=%d)\n",
		       ioc->name, __func__, __LINE__);
		return;
	}
	spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock, flags);
	if (ioc->fault_reset_work_q)
		queue_delayed_work(ioc->fault_reset_work_q,
				   &ioc->fault_reset_work,
				   msecs_to_jiffies(LEAPIORAID_FAULT_POLLING_INTERVAL));
	spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
	if (ioc->open_pcie_trace)
		leapioraid_base_start_log_watchdog(ioc);
}

void
leapioraid_base_stop_watchdog(struct LEAPIORAID_ADAPTER *ioc)
{
	unsigned long flags;
	struct workqueue_struct *wq;

	spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock, flags);
	wq = ioc->fault_reset_work_q;
	ioc->fault_reset_work_q = NULL;
	spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
	if (wq) {
		if (!cancel_delayed_work_sync(&ioc->fault_reset_work))
			flush_workqueue(wq);
		destroy_workqueue(wq);
	}
	if (ioc->open_pcie_trace)
		leapioraid_base_stop_log_watchdog(ioc);
}

void
leapioraid_base_start_hba_unplug_watchdog(struct LEAPIORAID_ADAPTER *ioc)
{
	unsigned long flags;

	if (ioc->hba_hot_unplug_work_q)
		return;
	INIT_DELAYED_WORK(&ioc->hba_hot_unplug_work,
		leapioraid_base_hba_hot_unplug_work);
	snprintf(ioc->hba_hot_unplug_work_q_name,
		 sizeof(ioc->hba_hot_unplug_work_q_name),
		 "poll_%s%u_hba_unplug", ioc->driver_name, ioc->id);
	ioc->hba_hot_unplug_work_q =
	    create_singlethread_workqueue(ioc->hba_hot_unplug_work_q_name);
	if (!ioc->hba_hot_unplug_work_q) {
		pr_err("%s %s: failed (line=%d)\n",
		       ioc->name, __func__, __LINE__);
		return;
	}
	spin_lock_irqsave(&ioc->hba_hot_unplug_lock, flags);
	if (ioc->hba_hot_unplug_work_q)
		queue_delayed_work(ioc->hba_hot_unplug_work_q,
				   &ioc->hba_hot_unplug_work,
				   msecs_to_jiffies(LEAPIORAID_FAULT_POLLING_INTERVAL));
	spin_unlock_irqrestore(&ioc->hba_hot_unplug_lock, flags);
}

void
leapioraid_base_stop_hba_unplug_watchdog(struct LEAPIORAID_ADAPTER *ioc)
{
	unsigned long flags;
	struct workqueue_struct *wq;

	spin_lock_irqsave(&ioc->hba_hot_unplug_lock, flags);
	wq = ioc->hba_hot_unplug_work_q;
	ioc->hba_hot_unplug_work_q = NULL;
	spin_unlock_irqrestore(&ioc->hba_hot_unplug_lock, flags);
	if (wq) {
		if (!cancel_delayed_work_sync(&ioc->hba_hot_unplug_work))
			flush_workqueue(wq);
		destroy_workqueue(wq);
	}
}

static void
leapioraid_base_stop_smart_polling(struct LEAPIORAID_ADAPTER *ioc)
{
	struct workqueue_struct *wq;

	wq = ioc->smart_poll_work_q;
	ioc->smart_poll_work_q = NULL;
	if (wq) {
		if (!cancel_delayed_work(&ioc->smart_poll_work))
			flush_workqueue(wq);
		destroy_workqueue(wq);
	}
}

void
leapioraid_base_fault_info(struct LEAPIORAID_ADAPTER *ioc, u16 fault_code)
{
	pr_err("%s fault_state(0x%04x)!\n",
	       ioc->name, fault_code);
}

void
leapioraid_base_coredump_info(struct LEAPIORAID_ADAPTER *ioc, u16 fault_code)
{
	pr_err("%s coredump_state(0x%04x)!\n",
	       ioc->name, fault_code);
}

int
leapioraid_base_wait_for_coredump_completion(struct LEAPIORAID_ADAPTER *ioc,
					     const char *caller)
{
	u8 timeout =
	    (ioc->manu_pg11.CoreDumpTOSec) ? ioc->manu_pg11.CoreDumpTOSec : 15;
	int ioc_state =
	    leapioraid_base_wait_on_iocstate(ioc, LEAPIORAID_IOC_STATE_FAULT,
					     timeout);

	if (ioc_state)
		pr_err("%s %s: CoreDump timed out. (ioc_state=0x%x)\n",
			ioc->name, caller, ioc_state);
	else
		pr_info("%s %s: CoreDump completed. (ioc_state=0x%x)\n",
			ioc->name, caller, ioc_state);
	return ioc_state;
}

void
leapioraid_halt_firmware(struct LEAPIORAID_ADAPTER *ioc, u8 set_fault)
{
	u32 doorbell;

	if ((!ioc->fwfault_debug) && (!set_fault))
		return;
	if (!set_fault)
		dump_stack();
	doorbell =
	    ioc->base_readl(&ioc->chip->Doorbell,
			LEAPIORAID_READL_RETRY_COUNT_OF_THIRTY);
	if ((doorbell & LEAPIORAID_IOC_STATE_MASK)
		== LEAPIORAID_IOC_STATE_FAULT) {
		leapioraid_print_fault_code(ioc, doorbell);
	} else if ((doorbell & LEAPIORAID_IOC_STATE_MASK) ==
		   LEAPIORAID_IOC_STATE_COREDUMP)
		leapioraid_base_coredump_info(ioc,
					      doorbell &
					      LEAPIORAID_DOORBELL_DATA_MASK);
	else {
		writel(0xC0FFEE00, &ioc->chip->Doorbell);
		if (!set_fault)
			pr_err("%s Firmware is halted due to command timeout\n",
				ioc->name);
	}
	if (set_fault)
		return;
	if (ioc->fwfault_debug == 2) {
		for (;;)
			;
	} else
		panic("panic in %s\n", __func__);
}

static void
leapioraid_base_group_cpus_on_irq(struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_adapter_reply_queue *reply_q;
	unsigned int i, cpu, group, nr_cpus, nr_msix, index = 0;
	int iopoll_q_count = ioc->reply_queue_count - ioc->iopoll_q_start_index;
	int unmanaged_q_count = ioc->high_iops_queues + iopoll_q_count;

	cpu = cpumask_first(cpu_online_mask);
	nr_msix = ioc->reply_queue_count - unmanaged_q_count;
	nr_cpus = num_online_cpus();
	group = nr_cpus / nr_msix;
	list_for_each_entry(reply_q, &ioc->reply_queue_list, list) {
		if (reply_q->msix_index < ioc->high_iops_queues ||
		    reply_q->msix_index >= ioc->iopoll_q_start_index)
			continue;
		if (cpu >= nr_cpus)
			break;
		if (index < nr_cpus % nr_msix)
			group++;
		for (i = 0; i < group; i++) {
			ioc->cpu_msix_table[cpu] = reply_q->msix_index;
			cpu = cpumask_next(cpu, cpu_online_mask);
		}
		index++;
	}
}

static void
leapioraid_base_sas_ioc_info(struct LEAPIORAID_ADAPTER *ioc,
			     struct LeapioraidDefaultRep_t *mpi_reply,
			     struct LeapioraidReqHeader_t *request_hdr)
{
	u16 ioc_status = le16_to_cpu(mpi_reply->IOCStatus) &
	    LEAPIORAID_IOCSTATUS_MASK;
	char *desc = NULL;
	u16 frame_sz;
	char *func_str = NULL;

	if (request_hdr->Function == LEAPIORAID_FUNC_SCSI_IO_REQUEST ||
	    request_hdr->Function == LEAPIORAID_FUNC_RAID_SCSI_IO_PASSTHROUGH
	    || request_hdr->Function == LEAPIORAID_FUNC_EVENT_NOTIFICATION)
		return;
	if (ioc_status == LEAPIORAID_IOCSTATUS_CONFIG_INVALID_PAGE)
		return;
	switch (ioc_status) {
	case LEAPIORAID_IOCSTATUS_INVALID_FUNCTION:
		desc = "invalid function";
		break;
	case LEAPIORAID_IOCSTATUS_BUSY:
		desc = "busy";
		break;
	case LEAPIORAID_IOCSTATUS_INVALID_SGL:
		desc = "invalid sgl";
		break;
	case LEAPIORAID_IOCSTATUS_INTERNAL_ERROR:
		desc = "internal error";
		break;
	case LEAPIORAID_IOCSTATUS_INVALID_VPID:
		desc = "invalid vpid";
		break;
	case LEAPIORAID_IOCSTATUS_INSUFFICIENT_RESOURCES:
		desc = "insufficient resources";
		break;
	case LEAPIORAID_IOCSTATUS_INSUFFICIENT_POWER:
		desc = "insufficient power";
		break;
	case LEAPIORAID_IOCSTATUS_INVALID_FIELD:
		desc = "invalid field";
		break;
	case LEAPIORAID_IOCSTATUS_INVALID_STATE:
		desc = "invalid state";
		break;
	case LEAPIORAID_IOCSTATUS_OP_STATE_NOT_SUPPORTED:
		desc = "op state not supported";
		break;
	case LEAPIORAID_IOCSTATUS_CONFIG_INVALID_ACTION:
		desc = "config invalid action";
		break;
	case LEAPIORAID_IOCSTATUS_CONFIG_INVALID_TYPE:
		desc = "config invalid type";
		break;
	case LEAPIORAID_IOCSTATUS_CONFIG_INVALID_DATA:
		desc = "config invalid data";
		break;
	case LEAPIORAID_IOCSTATUS_CONFIG_NO_DEFAULTS:
		desc = "config no defaults";
		break;
	case LEAPIORAID_IOCSTATUS_CONFIG_CANT_COMMIT:
		desc = "config can not commit";
		break;
	case LEAPIORAID_IOCSTATUS_SCSI_RECOVERED_ERROR:
	case LEAPIORAID_IOCSTATUS_SCSI_INVALID_DEVHANDLE:
	case LEAPIORAID_IOCSTATUS_SCSI_DEVICE_NOT_THERE:
	case LEAPIORAID_IOCSTATUS_SCSI_DATA_OVERRUN:
	case LEAPIORAID_IOCSTATUS_SCSI_DATA_UNDERRUN:
	case LEAPIORAID_IOCSTATUS_SCSI_IO_DATA_ERROR:
	case LEAPIORAID_IOCSTATUS_SCSI_PROTOCOL_ERROR:
	case LEAPIORAID_IOCSTATUS_SCSI_TASK_TERMINATED:
	case LEAPIORAID_IOCSTATUS_SCSI_RESIDUAL_MISMATCH:
	case LEAPIORAID_IOCSTATUS_SCSI_TASK_MGMT_FAILED:
	case LEAPIORAID_IOCSTATUS_SCSI_IOC_TERMINATED:
	case LEAPIORAID_IOCSTATUS_SCSI_EXT_TERMINATED:
		break;
	case LEAPIORAID_IOCSTATUS_EEDP_GUARD_ERROR:
		if (!ioc->disable_eedp_support)
			desc = "eedp guard error";
		break;
	case LEAPIORAID_IOCSTATUS_EEDP_REF_TAG_ERROR:
		if (!ioc->disable_eedp_support)
			desc = "eedp ref tag error";
		break;
	case LEAPIORAID_IOCSTATUS_EEDP_APP_TAG_ERROR:
		if (!ioc->disable_eedp_support)
			desc = "eedp app tag error";
		break;
	case LEAPIORAID_IOCSTATUS_TARGET_INVALID_IO_INDEX:
		desc = "target invalid io index";
		break;
	case LEAPIORAID_IOCSTATUS_TARGET_ABORTED:
		desc = "target aborted";
		break;
	case LEAPIORAID_IOCSTATUS_TARGET_NO_CONN_RETRYABLE:
		desc = "target no conn retryable";
		break;
	case LEAPIORAID_IOCSTATUS_TARGET_NO_CONNECTION:
		desc = "target no connection";
		break;
	case LEAPIORAID_IOCSTATUS_TARGET_XFER_COUNT_MISMATCH:
		desc = "target xfer count mismatch";
		break;
	case LEAPIORAID_IOCSTATUS_TARGET_DATA_OFFSET_ERROR:
		desc = "target data offset error";
		break;
	case LEAPIORAID_IOCSTATUS_TARGET_TOO_MUCH_WRITE_DATA:
		desc = "target too much write data";
		break;
	case LEAPIORAID_IOCSTATUS_TARGET_IU_TOO_SHORT:
		desc = "target iu too short";
		break;
	case LEAPIORAID_IOCSTATUS_TARGET_ACK_NAK_TIMEOUT:
		desc = "target ack nak timeout";
		break;
	case LEAPIORAID_IOCSTATUS_TARGET_NAK_RECEIVED:
		desc = "target nak received";
		break;
	case LEAPIORAID_IOCSTATUS_SAS_SMP_REQUEST_FAILED:
		desc = "smp request failed";
		break;
	case LEAPIORAID_IOCSTATUS_SAS_SMP_DATA_OVERRUN:
		desc = "smp data overrun";
		break;
	default:
		break;
	}
	if (!desc)
		return;
	switch (request_hdr->Function) {
	case LEAPIORAID_FUNC_CONFIG:
		frame_sz = sizeof(struct LeapioraidCfgReq_t) + ioc->sge_size;
		func_str = "config_page";
		break;
	case LEAPIORAID_FUNC_SCSI_TASK_MGMT:
		frame_sz = sizeof(struct LeapioraidSCSITmgReq_t);
		func_str = "task_mgmt";
		break;
	case LEAPIORAID_FUNC_SAS_IO_UNIT_CONTROL:
		frame_sz = sizeof(struct LeapioraidSasIoUnitControlReq_t);
		func_str = "sas_iounit_ctl";
		break;
	case LEAPIORAID_FUNC_SCSI_ENCLOSURE_PROCESSOR:
		frame_sz = sizeof(struct LeapioraidSepReq_t);
		func_str = "enclosure";
		break;
	case LEAPIORAID_FUNC_IOC_INIT:
		frame_sz = sizeof(struct LeapioraidIOCInitReq_t);
		func_str = "ioc_init";
		break;
	case LEAPIORAID_FUNC_PORT_ENABLE:
		frame_sz = sizeof(struct LeapioraidPortEnableReq_t);
		func_str = "port_enable";
		break;
	case LEAPIORAID_FUNC_SMP_PASSTHROUGH:
		frame_sz =
		    sizeof(struct LeapioraidSmpPassthroughReq_t) + ioc->sge_size;
		func_str = "smp_passthru";
		break;
	default:
		frame_sz = 32;
		func_str = "unknown";
		break;
	}
	pr_warn("%s ioc_status: %s(0x%04x), request(0x%p), (%s)\n",
		ioc->name, desc, ioc_status, request_hdr, func_str);
	leapioraid_debug_dump_mf(request_hdr, frame_sz / 4);
}

static void
leapioraid_base_display_event_data(struct LEAPIORAID_ADAPTER *ioc,
				   struct LeapioraidEventNotificationRep_t *mpi_reply)
{
	char *desc = NULL;
	u16 event;

	if (!(ioc->logging_level & LEAPIORAID_DEBUG_EVENTS))
		return;
	event = le16_to_cpu(mpi_reply->Event);
	if (ioc->warpdrive_msg) {
		switch (event) {
		case LEAPIORAID_EVENT_IR_OPERATION_STATUS:
		case LEAPIORAID_EVENT_IR_VOLUME:
		case LEAPIORAID_EVENT_IR_PHYSICAL_DISK:
		case LEAPIORAID_EVENT_IR_CONFIGURATION_CHANGE_LIST:
		case LEAPIORAID_EVENT_LOG_ENTRY_ADDED:
			return;
		}
	}
	switch (event) {
	case LEAPIORAID_EVENT_LOG_DATA:
		desc = "Log Data";
		break;
	case LEAPIORAID_EVENT_STATE_CHANGE:
		desc = "Status Change";
		break;
	case LEAPIORAID_EVENT_HARD_RESET_RECEIVED:
		desc = "Hard Reset Received";
		break;
	case LEAPIORAID_EVENT_EVENT_CHANGE:
		desc = "Event Change";
		break;
	case LEAPIORAID_EVENT_SAS_DEVICE_STATUS_CHANGE:
		desc = "Device Status Change";
		break;
	case LEAPIORAID_EVENT_IR_OPERATION_STATUS:
		desc = "IR Operation Status";
		break;
	case LEAPIORAID_EVENT_SAS_DISCOVERY:
		{
			struct LeapioraidEventDataSasDiscovery_t *event_data =
				(struct LeapioraidEventDataSasDiscovery_t *) mpi_reply->EventData;
			pr_info("%s SAS Discovery: (%s)",
			       ioc->name,
			       (event_data->ReasonCode ==
				LEAPIORAID_EVENT_SAS_DISC_RC_STARTED) ? "start" :
			       "stop");
			if (event_data->DiscoveryStatus)
				pr_info("discovery_status(0x%08x)",
				       le32_to_cpu(event_data->DiscoveryStatus));
			pr_info("\n");
			return;
		}
	case LEAPIORAID_EVENT_SAS_BROADCAST_PRIMITIVE:
		desc = "SAS Broadcast Primitive";
		break;
	case LEAPIORAID_EVENT_SAS_INIT_DEVICE_STATUS_CHANGE:
		desc = "SAS Init Device Status Change";
		break;
	case LEAPIORAID_EVENT_SAS_INIT_TABLE_OVERFLOW:
		desc = "SAS Init Table Overflow";
		break;
	case LEAPIORAID_EVENT_SAS_TOPOLOGY_CHANGE_LIST:
		desc = "SAS Topology Change List";
		break;
	case LEAPIORAID_EVENT_SAS_ENCL_DEVICE_STATUS_CHANGE:
		desc = "SAS Enclosure Device Status Change";
		break;
	case LEAPIORAID_EVENT_IR_VOLUME:
		desc = "IR Volume";
		break;
	case LEAPIORAID_EVENT_IR_PHYSICAL_DISK:
		desc = "IR Physical Disk";
		break;
	case LEAPIORAID_EVENT_IR_CONFIGURATION_CHANGE_LIST:
		desc = "IR Configuration Change List";
		break;
	case LEAPIORAID_EVENT_LOG_ENTRY_ADDED:
		desc = "Log Entry Added";
		break;
	case LEAPIORAID_EVENT_TEMP_THRESHOLD:
		desc = "Temperature Threshold";
		break;
	case LEAPIORAID_EVENT_SAS_DEVICE_DISCOVERY_ERROR:
		desc = "SAS Device Discovery Error";
		break;
	}
	if (!desc)
		return;
	pr_info("%s %s\n", ioc->name, desc);
}

static void
leapioraid_base_sas_log_info(struct LEAPIORAID_ADAPTER *ioc, u32 log_info)
{
	union loginfo_type {
		u32 loginfo;
		struct {
			u32 subcode:16;
			u32 code:8;
			u32 originator:4;
			u32 bus_type:4;
		} dw;
	};
	union loginfo_type sas_loginfo;
	char *originator_str = NULL;

	sas_loginfo.loginfo = log_info;
	if (sas_loginfo.dw.bus_type != 3)
		return;
	if (log_info == 0x31170000)
		return;
	if (ioc->ignore_loginfos && (log_info == 0x30050000 || log_info ==
				     0x31140000 || log_info == 0x31130000))
		return;
	switch (sas_loginfo.dw.originator) {
	case 0:
		originator_str = "IOP";
		break;
	case 1:
		originator_str = "PL";
		break;
	case 2:
		if (ioc->warpdrive_msg)
			originator_str = "WarpDrive";
		else
			originator_str = "IR";
		break;
	}
	pr_warn("%s log_info(0x%08x):\n\t\t"
		"originator(%s), code(0x%02x), sub_code(0x%04x)\n",
		ioc->name,
		log_info,
		originator_str,
		sas_loginfo.dw.code,
		sas_loginfo.dw.subcode);
}

static void
leapioraid_base_display_reply_info(struct LEAPIORAID_ADAPTER *ioc, u16 smid,
				   u8 msix_index, u32 reply)
{
	struct LeapioraidDefaultRep_t *mpi_reply;
	u16 ioc_status;
	u32 loginfo = 0;

	mpi_reply = leapioraid_base_get_reply_virt_addr(ioc, reply);
	if (unlikely(!mpi_reply)) {
		pr_err(
		       "%s mpi_reply not valid at %s:%d/%s()!\n", ioc->name,
		       __FILE__, __LINE__, __func__);
		return;
	}
	ioc_status = le16_to_cpu(mpi_reply->IOCStatus);
	if ((ioc_status & LEAPIORAID_IOCSTATUS_MASK) &&
	    (ioc->logging_level & LEAPIORAID_DEBUG_REPLY)) {
		leapioraid_base_sas_ioc_info(ioc, mpi_reply,
					     leapioraid_base_get_msg_frame(ioc,
									   smid));
	}
	if (ioc_status & LEAPIORAID_IOCSTATUS_FLAG_LOG_INFO_AVAILABLE) {
		loginfo = le32_to_cpu(mpi_reply->IOCLogInfo);
		leapioraid_base_sas_log_info(ioc, loginfo);
	}
}

u8
leapioraid_base_done(struct LEAPIORAID_ADAPTER *ioc, u16 smid, u8 msix_index,
		     u32 reply)
{
	struct LeapioraidDefaultRep_t *mpi_reply;

	mpi_reply = leapioraid_base_get_reply_virt_addr(ioc, reply);
	if (mpi_reply && mpi_reply->Function == LEAPIORAID_FUNC_EVENT_ACK)
		return leapioraid_check_for_pending_internal_cmds(ioc, smid);
	if (ioc->base_cmds.status == LEAPIORAID_CMD_NOT_USED)
		return 1;
	ioc->base_cmds.status |= LEAPIORAID_CMD_COMPLETE;
	if (mpi_reply) {
		ioc->base_cmds.status |= LEAPIORAID_CMD_REPLY_VALID;
		memcpy(ioc->base_cmds.reply, mpi_reply,
		       mpi_reply->MsgLength * 4);
	}
	ioc->base_cmds.status &= ~LEAPIORAID_CMD_PENDING;
	complete(&ioc->base_cmds.done);
	return 1;
}

static u8
leapioraid_base_async_event(
	struct LEAPIORAID_ADAPTER *ioc, u8 msix_index, u32 reply)
{
	struct LeapioraidEventNotificationRep_t *mpi_reply;
	struct LeapioraidEventAckReq_t *ack_request;
	u16 smid;
	struct leapioraid_event_ack_list *delayed_event_ack;

	mpi_reply = leapioraid_base_get_reply_virt_addr(ioc, reply);
	if (!mpi_reply)
		return 1;
	if (mpi_reply->Function != LEAPIORAID_FUNC_EVENT_NOTIFICATION)
		return 1;
	leapioraid_base_display_event_data(ioc, mpi_reply);
	if (!(mpi_reply->AckRequired & LEAPIORAID_EVENT_NOTIFICATION_ACK_REQUIRED))
		goto out;
	smid = leapioraid_base_get_smid(ioc, ioc->base_cb_idx);
	if (!smid) {
		delayed_event_ack =
		    kzalloc(sizeof(*delayed_event_ack), GFP_ATOMIC);
		if (!delayed_event_ack)
			goto out;
		INIT_LIST_HEAD(&delayed_event_ack->list);
		delayed_event_ack->Event = mpi_reply->Event;
		delayed_event_ack->EventContext = mpi_reply->EventContext;
		list_add_tail(&delayed_event_ack->list,
			      &ioc->delayed_event_ack_list);
		dewtprintk(ioc, pr_err(
				       "%s DELAYED: EVENT ACK: event (0x%04x)\n",
				       ioc->name,
				       le16_to_cpu(mpi_reply->Event)));
		goto out;
	}
	ack_request = leapioraid_base_get_msg_frame(ioc, smid);
	memset(ack_request, 0, sizeof(struct LeapioraidEventAckReq_t));
	ack_request->Function = LEAPIORAID_FUNC_EVENT_ACK;
	ack_request->Event = mpi_reply->Event;
	ack_request->EventContext = mpi_reply->EventContext;
	ack_request->VF_ID = 0;
	ack_request->VP_ID = 0;
	ioc->put_smid_default(ioc, smid);
out:
	leapioraid_scsihost_event_callback(ioc, msix_index, reply);
	leapioraid_ctl_event_callback(ioc, msix_index, reply);
	return 1;
}

inline
struct leapioraid_scsiio_tracker *leapioraid_base_scsi_cmd_priv(
	struct scsi_cmnd *scmd)
{
	return scsi_cmd_priv(scmd);
}

struct leapioraid_scsiio_tracker *leapioraid_get_st_from_smid(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid)
{
	struct scsi_cmnd *cmd;

	if (WARN_ON(!smid) || WARN_ON(smid >= ioc->hi_priority_smid))
		return NULL;
	cmd = leapioraid_scsihost_scsi_lookup_get(ioc, smid);
	if (cmd)
		return leapioraid_base_scsi_cmd_priv(cmd);
	return NULL;
}

static u8
leapioraid_base_get_cb_idx(struct LEAPIORAID_ADAPTER *ioc, u16 smid)
{
	int i;
	u16 ctl_smid = ioc->shost->can_queue + LEAPIORAID_INTERNAL_SCSIIO_FOR_IOCTL;
	u16 discovery_smid =
	    ioc->shost->can_queue + LEAPIORAID_INTERNAL_SCSIIO_FOR_DISCOVERY;
	u8 cb_idx = 0xFF;

	if (smid < ioc->hi_priority_smid) {
		struct leapioraid_scsiio_tracker *st;

		if (smid < ctl_smid) {
			st = leapioraid_get_st_from_smid(ioc, smid);
			if (st)
				cb_idx = st->cb_idx;
		} else if (smid < discovery_smid)
			cb_idx = ioc->ctl_cb_idx;
		else
			cb_idx = ioc->scsih_cb_idx;
	} else if (smid < ioc->internal_smid) {
		i = smid - ioc->hi_priority_smid;
		cb_idx = ioc->hpr_lookup[i].cb_idx;
	} else if (smid <= ioc->hba_queue_depth) {
		i = smid - ioc->internal_smid;
		cb_idx = ioc->internal_lookup[i].cb_idx;
	}
	return cb_idx;
}

void
leapioraid_base_pause_mq_polling(struct LEAPIORAID_ADAPTER *ioc)
{
	int iopoll_q_count = ioc->reply_queue_count - ioc->iopoll_q_start_index;
	int qid;

	for (qid = 0; qid < iopoll_q_count; qid++)
		atomic_set(&ioc->blk_mq_poll_queues[qid].pause, 1);
	for (qid = 0; qid < iopoll_q_count; qid++) {
		while (atomic_read(&ioc->blk_mq_poll_queues[qid].busy)) {
			cpu_relax();
			udelay(500);
		}
	}
}

void
leapioraid_base_resume_mq_polling(struct LEAPIORAID_ADAPTER *ioc)
{
	int iopoll_q_count = ioc->reply_queue_count - ioc->iopoll_q_start_index;
	int qid;

	for (qid = 0; qid < iopoll_q_count; qid++)
		atomic_set(&ioc->blk_mq_poll_queues[qid].pause, 0);
}

void
leapioraid_base_mask_interrupts(struct LEAPIORAID_ADAPTER *ioc)
{
	u32 him_register;

	ioc->mask_interrupts = 1;
	him_register =
	    ioc->base_readl(&ioc->chip->HostInterruptMask,
			    LEAPIORAID_READL_RETRY_COUNT_OF_THREE);
	him_register |=
	    0x00000001 + 0x00000008 + 0x40000000;
	writel(him_register, &ioc->chip->HostInterruptMask);
	ioc->base_readl(&ioc->chip->HostInterruptMask,
			LEAPIORAID_READL_RETRY_COUNT_OF_THREE);
}

void
leapioraid_base_unmask_interrupts(struct LEAPIORAID_ADAPTER *ioc)
{
	u32 him_register;

	him_register =
	    ioc->base_readl(&ioc->chip->HostInterruptMask,
			    LEAPIORAID_READL_RETRY_COUNT_OF_THREE);
	him_register &= ~0x00000008;
	writel(him_register, &ioc->chip->HostInterruptMask);
	ioc->mask_interrupts = 0;
}

union leapioraid_reply_descriptor {
	u64 word;
	struct {
		u32 low;
		u32 high;
	} u;
};

static int
leapioraid_base_process_reply_queue(
	struct leapioraid_adapter_reply_queue *reply_q)
{
	union leapioraid_reply_descriptor rd;
	u64 completed_cmds;
	u8 request_descript_type;
	u16 smid;
	u8 cb_idx;
	u32 reply;
	u8 msix_index = reply_q->msix_index;
	struct LEAPIORAID_ADAPTER *ioc = reply_q->ioc;
	union LeapioraidRepDescUnion_t *rpf;
	u8 rc;

	completed_cmds = 0;
	if (!atomic_add_unless(&reply_q->busy, 1, 1))
		return completed_cmds;
	rpf = &reply_q->reply_post_free[reply_q->reply_post_host_index];
	request_descript_type = rpf->Default.ReplyFlags
	    & LEAPIORAID_RPY_DESCRIPT_FLAGS_TYPE_MASK;
	if (request_descript_type == LEAPIORAID_RPY_DESCRIPT_FLAGS_UNUSED) {
		atomic_dec(&reply_q->busy);
		return 1;
	}
	cb_idx = 0xFF;
	do {
		rd.word = le64_to_cpu(rpf->Words);
		if (rd.u.low == UINT_MAX || rd.u.high == UINT_MAX)
			goto out;
		reply = 0;
		smid = le16_to_cpu(rpf->Default.DescriptorTypeDependent1);
		if (request_descript_type ==
		    LEAPIORAID_RPY_DESCRIPT_FLAGS_FAST_PATH_SCSI_IO_SUCCESS ||
		    request_descript_type ==
		    LEAPIORAID_RPY_DESCRIPT_FLAGS_SCSI_IO_SUCCESS) {
			cb_idx = leapioraid_base_get_cb_idx(ioc, smid);
			if ((likely(cb_idx < LEAPIORAID_MAX_CALLBACKS)) &&
			    (likely(leapioraid_callbacks[cb_idx] != NULL))) {
				rc = leapioraid_callbacks[cb_idx] (ioc, smid,
								msix_index, 0);
				if (rc)
					leapioraid_base_free_smid(ioc, smid);
			}
		} else if (request_descript_type ==
			   LEAPIORAID_RPY_DESCRIPT_FLAGS_ADDRESS_REPLY) {
			reply =
			    le32_to_cpu(rpf->AddressReply.ReplyFrameAddress);
			if (reply > ioc->reply_dma_max_address
			    || reply < ioc->reply_dma_min_address)
				reply = 0;
			if (smid) {
				cb_idx = leapioraid_base_get_cb_idx(ioc, smid);
				if ((likely(cb_idx < LEAPIORAID_MAX_CALLBACKS)) &&
				    (likely(leapioraid_callbacks[cb_idx] != NULL))) {
					rc = leapioraid_callbacks[cb_idx] (ioc,
									smid,
									msix_index,
									reply);
					if (reply)
						leapioraid_base_display_reply_info
						    (ioc, smid, msix_index,
						     reply);
					if (rc)
						leapioraid_base_free_smid(ioc,
									  smid);
				}
			} else {
				leapioraid_base_async_event(ioc, msix_index, reply);
			}
			if (reply) {
				ioc->reply_free_host_index =
				    (ioc->reply_free_host_index ==
				     (ioc->reply_free_queue_depth - 1)) ?
				    0 : ioc->reply_free_host_index + 1;
				ioc->reply_free[ioc->reply_free_host_index] =
				    cpu_to_le32(reply);
				wmb(); /* Make sure that all write ops are in order */
				writel(ioc->reply_free_host_index,
				       &ioc->chip->ReplyFreeHostIndex);
			}
		}
		rpf->Words = cpu_to_le64(ULLONG_MAX);
		reply_q->reply_post_host_index =
		    (reply_q->reply_post_host_index ==
		     (ioc->reply_post_queue_depth - 1)) ? 0 :
		    reply_q->reply_post_host_index + 1;
		request_descript_type =
			reply_q->reply_post_free[reply_q->reply_post_host_index].Default.ReplyFlags
			& LEAPIORAID_RPY_DESCRIPT_FLAGS_TYPE_MASK;
		completed_cmds++;
		if (completed_cmds >= ioc->thresh_hold) {
			if (ioc->combined_reply_queue) {
				writel(reply_q->reply_post_host_index |
				       ((msix_index & 7) <<
					LEAPIORAID_RPHI_MSIX_INDEX_SHIFT),
				       ioc->replyPostRegisterIndex[msix_index /
								   8]);
			} else {
				writel(reply_q->reply_post_host_index |
				       (msix_index <<
					LEAPIORAID_RPHI_MSIX_INDEX_SHIFT),
				       &ioc->chip->ReplyPostHostIndex);
			}
			if (!reply_q->is_blk_mq_poll_q &&
			    !reply_q->irq_poll_scheduled) {
				reply_q->irq_poll_scheduled = true;
				irq_poll_sched(&reply_q->irqpoll);
			}
			atomic_dec(&reply_q->busy);
			return completed_cmds;
		}
		if (request_descript_type == LEAPIORAID_RPY_DESCRIPT_FLAGS_UNUSED)
			goto out;
		if (!reply_q->reply_post_host_index)
			rpf = reply_q->reply_post_free;
		else
			rpf++;
	} while (1);
out:
	if (!completed_cmds) {
		atomic_dec(&reply_q->busy);
		return completed_cmds;
	}
	wmb(); /* Make sure that all write ops are in order */
	if (ioc->combined_reply_queue) {
		writel(reply_q->reply_post_host_index | ((msix_index & 7) <<
							 LEAPIORAID_RPHI_MSIX_INDEX_SHIFT),
		       ioc->replyPostRegisterIndex[msix_index / 8]);
	} else {
		writel(reply_q->reply_post_host_index | (msix_index <<
							 LEAPIORAID_RPHI_MSIX_INDEX_SHIFT),
		       &ioc->chip->ReplyPostHostIndex);
	}
	atomic_dec(&reply_q->busy);
	return completed_cmds;
}

int leapioraid_blk_mq_poll(struct Scsi_Host *shost, unsigned int queue_num)
{
	struct LEAPIORAID_ADAPTER *ioc =
	    (struct LEAPIORAID_ADAPTER *)shost->hostdata;
	struct leapioraid_adapter_reply_queue *reply_q;
	int num_entries = 0;
	int qid = queue_num - ioc->iopoll_q_start_index;

	if (atomic_read(&ioc->blk_mq_poll_queues[qid].pause) ||
	    !atomic_add_unless(&ioc->blk_mq_poll_queues[qid].busy, 1, 1))
		return 0;
	reply_q = ioc->blk_mq_poll_queues[qid].reply_q;
	num_entries = leapioraid_base_process_reply_queue(reply_q);
	atomic_dec(&ioc->blk_mq_poll_queues[qid].busy);
	return num_entries;
}

static irqreturn_t
leapioraid_base_interrupt(int irq, void *bus_id)
{
	struct leapioraid_adapter_reply_queue *reply_q = bus_id;
	struct LEAPIORAID_ADAPTER *ioc = reply_q->ioc;

	if (ioc->mask_interrupts)
		return IRQ_NONE;
	if (reply_q->irq_poll_scheduled)
		return IRQ_HANDLED;
	return ((leapioraid_base_process_reply_queue(reply_q) > 0) ?
		IRQ_HANDLED : IRQ_NONE);
}

static
int leapioraid_base_irqpoll(struct irq_poll *irqpoll, int budget)
{
	struct leapioraid_adapter_reply_queue *reply_q;
	int num_entries = 0;

	reply_q = container_of(irqpoll,
		struct leapioraid_adapter_reply_queue, irqpoll);
	if (reply_q->irq_line_enable) {
		disable_irq_nosync(reply_q->os_irq);
		reply_q->irq_line_enable = false;
	}
	num_entries = leapioraid_base_process_reply_queue(reply_q);
	if (num_entries < budget) {
		irq_poll_complete(irqpoll);
		reply_q->irq_poll_scheduled = false;
		reply_q->irq_line_enable = true;
		enable_irq(reply_q->os_irq);
	}
	return num_entries;
}

static void
leapioraid_base_init_irqpolls(struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_adapter_reply_queue *reply_q, *next;

	if (list_empty(&ioc->reply_queue_list))
		return;
	list_for_each_entry_safe(reply_q, next, &ioc->reply_queue_list, list) {
		if (reply_q->is_blk_mq_poll_q)
			continue;
		irq_poll_init(&reply_q->irqpoll, ioc->thresh_hold,
			      leapioraid_base_irqpoll);
		reply_q->irq_poll_scheduled = false;
		reply_q->irq_line_enable = true;
		reply_q->os_irq = pci_irq_vector(ioc->pdev,
						 reply_q->msix_index);
	}
}

static inline int
leapioraid_base_is_controller_msix_enabled(struct LEAPIORAID_ADAPTER *ioc)
{
	return (ioc->facts.IOCCapabilities &
		LEAPIORAID_IOCFACTS_CAPABILITY_MSI_X_INDEX) && ioc->msix_enable;
}

void
leapioraid_base_sync_reply_irqs(struct LEAPIORAID_ADAPTER *ioc, u8 poll)
{
	struct leapioraid_adapter_reply_queue *reply_q;

	if (!leapioraid_base_is_controller_msix_enabled(ioc))
		return;
	list_for_each_entry(reply_q, &ioc->reply_queue_list, list) {
		if (ioc->shost_recovery || ioc->remove_host ||
		    ioc->pci_error_recovery)
			return;
		if (reply_q->msix_index == 0)
			continue;
		if (reply_q->is_blk_mq_poll_q) {
			leapioraid_base_process_reply_queue(reply_q);
			continue;
		}
		synchronize_irq(pci_irq_vector(ioc->pdev, reply_q->msix_index));
		if (reply_q->irq_poll_scheduled) {
			irq_poll_disable(&reply_q->irqpoll);
			irq_poll_enable(&reply_q->irqpoll);
			if (reply_q->irq_poll_scheduled) {
				reply_q->irq_poll_scheduled = false;
				reply_q->irq_line_enable = true;
				enable_irq(reply_q->os_irq);
			}
		}
		if (poll)
			leapioraid_base_process_reply_queue(reply_q);
	}
}

void
leapioraid_base_release_callback_handler(u8 cb_idx)
{
	leapioraid_callbacks[cb_idx] = NULL;
}

u8
leapioraid_base_register_callback_handler(LEAPIORAID_CALLBACK cb_func)
{
	u8 cb_idx;

	for (cb_idx = LEAPIORAID_MAX_CALLBACKS - 1; cb_idx; cb_idx--)
		if (leapioraid_callbacks[cb_idx] == NULL)
			break;
	leapioraid_callbacks[cb_idx] = cb_func;
	return cb_idx;
}

void
leapioraid_base_initialize_callback_handler(void)
{
	u8 cb_idx;

	for (cb_idx = 0; cb_idx < LEAPIORAID_MAX_CALLBACKS; cb_idx++)
		leapioraid_base_release_callback_handler(cb_idx);
}

static void
leapioraid_base_build_zero_len_sge(
	struct LEAPIORAID_ADAPTER *ioc, void *paddr)
{
	u32 flags_length = (u32) ((LEAPIORAID_SGE_FLAGS_LAST_ELEMENT |
				   LEAPIORAID_SGE_FLAGS_END_OF_BUFFER |
				   LEAPIORAID_SGE_FLAGS_END_OF_LIST |
				   LEAPIORAID_SGE_FLAGS_SIMPLE_ELEMENT) <<
				  LEAPIORAID_SGE_FLAGS_SHIFT);

	ioc->base_add_sg_single(paddr, flags_length, -1);
}

static void
leapioraid_base_add_sg_single_32(void *paddr, u32 flags_length,
				 dma_addr_t dma_addr)
{
	struct LeapioSGESimple32_t *sgel = paddr;

	flags_length |= (LEAPIORAID_SGE_FLAGS_32_BIT_ADDRESSING |
			 LEAPIORAID_SGE_FLAGS_SYSTEM_ADDRESS) <<
	    LEAPIORAID_SGE_FLAGS_SHIFT;
	sgel->FlagsLength = cpu_to_le32(flags_length);
	sgel->Address = cpu_to_le32(dma_addr);
}

static void
leapioraid_base_add_sg_single_64(void *paddr, u32 flags_length,
				 dma_addr_t dma_addr)
{
	struct LeapioSGESimple64_t *sgel = paddr;

	flags_length |= (LEAPIORAID_SGE_FLAGS_64_BIT_ADDRESSING |
			 LEAPIORAID_SGE_FLAGS_SYSTEM_ADDRESS) <<
	    LEAPIORAID_SGE_FLAGS_SHIFT;
	sgel->FlagsLength = cpu_to_le32(flags_length);
	sgel->Address = cpu_to_le64(dma_addr);
}

static
struct leapioraid_chain_tracker *leapioraid_base_get_chain_buffer_tracker(
	struct LEAPIORAID_ADAPTER *ioc,
	struct scsi_cmnd *scmd)
{
	struct leapioraid_chain_tracker *chain_req;
	struct leapioraid_scsiio_tracker *st = leapioraid_base_scsi_cmd_priv(scmd);
	u16 smid = st->smid;
	u8 chain_offset =
	    atomic_read(&ioc->chain_lookup[smid - 1].chain_offset);

	if (chain_offset == ioc->chains_needed_per_io)
		return NULL;
	chain_req = &ioc->chain_lookup[smid - 1].chains_per_smid[chain_offset];
	atomic_inc(&ioc->chain_lookup[smid - 1].chain_offset);
	return chain_req;
}

static void
leapioraid_base_build_sg(struct LEAPIORAID_ADAPTER *ioc, void *psge,
			 dma_addr_t data_out_dma, size_t data_out_sz,
			 dma_addr_t data_in_dma, size_t data_in_sz)
{
	u32 sgl_flags;

	if (!data_out_sz && !data_in_sz) {
		leapioraid_base_build_zero_len_sge(ioc, psge);
		return;
	}
	if (data_out_sz && data_in_sz) {
		sgl_flags = (LEAPIORAID_SGE_FLAGS_SIMPLE_ELEMENT |
			     LEAPIORAID_SGE_FLAGS_END_OF_BUFFER |
			     LEAPIORAID_SGE_FLAGS_HOST_TO_IOC);
		sgl_flags = sgl_flags << LEAPIORAID_SGE_FLAGS_SHIFT;
		ioc->base_add_sg_single(psge, sgl_flags |
					data_out_sz, data_out_dma);
		psge += ioc->sge_size;
		sgl_flags = (LEAPIORAID_SGE_FLAGS_SIMPLE_ELEMENT |
			     LEAPIORAID_SGE_FLAGS_LAST_ELEMENT |
			     LEAPIORAID_SGE_FLAGS_END_OF_BUFFER |
			     LEAPIORAID_SGE_FLAGS_END_OF_LIST);
		sgl_flags = sgl_flags << LEAPIORAID_SGE_FLAGS_SHIFT;
		ioc->base_add_sg_single(psge, sgl_flags |
					data_in_sz, data_in_dma);
	} else if (data_out_sz) {
		sgl_flags = (LEAPIORAID_SGE_FLAGS_SIMPLE_ELEMENT |
			     LEAPIORAID_SGE_FLAGS_LAST_ELEMENT |
			     LEAPIORAID_SGE_FLAGS_END_OF_BUFFER |
			     LEAPIORAID_SGE_FLAGS_END_OF_LIST |
			     LEAPIORAID_SGE_FLAGS_HOST_TO_IOC);
		sgl_flags = sgl_flags << LEAPIORAID_SGE_FLAGS_SHIFT;
		ioc->base_add_sg_single(psge, sgl_flags |
					data_out_sz, data_out_dma);
	} else if (data_in_sz) {
		sgl_flags = (LEAPIORAID_SGE_FLAGS_SIMPLE_ELEMENT |
			     LEAPIORAID_SGE_FLAGS_LAST_ELEMENT |
			     LEAPIORAID_SGE_FLAGS_END_OF_BUFFER |
			     LEAPIORAID_SGE_FLAGS_END_OF_LIST);
		sgl_flags = sgl_flags << LEAPIORAID_SGE_FLAGS_SHIFT;
		ioc->base_add_sg_single(psge, sgl_flags |
					data_in_sz, data_in_dma);
	}
}

u32
leapioraid_base_mod64(u64 dividend, u32 divisor)
{
	u32 remainder;

	if (!divisor) {
		pr_err("leapioraid : DIVISOR is zero, in div fn\n");
		return 0;
	}
	remainder = do_div(dividend, divisor);
	return remainder;
}

static void
leapioraid_base_add_sg_single_ieee(void *paddr, u8 flags, u8 chain_offset,
				   u32 length, dma_addr_t dma_addr)
{
	struct LEAPIORAID_IEEE_SGE_CHAIN64 *sgel = paddr;

	sgel->Flags = flags;
	sgel->NextChainOffset = chain_offset;
	sgel->Length = cpu_to_le32(length);
	sgel->Address = cpu_to_le64(dma_addr);
}

static void
leapioraid_base_build_zero_len_sge_ieee(struct LEAPIORAID_ADAPTER *ioc,
					void *paddr)
{
	u8 sgl_flags = (LEAPIORAID_IEEE_SGE_FLAGS_SIMPLE_ELEMENT |
			LEAPIORAID_IEEE_SGE_FLAGS_SYSTEM_ADDR |
			LEAPIORAID_IEEE_SGE_FLAGS_END_OF_LIST);

	leapioraid_base_add_sg_single_ieee(paddr, sgl_flags, 0, 0, -1);
}

static int
leapioraid_base_build_sg_scmd_ieee(struct LEAPIORAID_ADAPTER *ioc,
				   struct scsi_cmnd *scmd, u16 smid)
{
	struct LeapioraidSCSIIOReq_t *mpi_request;
	dma_addr_t chain_dma;
	struct scatterlist *sg_scmd;
	void *sg_local, *chain;
	u32 chain_offset;
	u32 chain_length;
	int sges_left;
	u32 sges_in_segment;
	u8 simple_sgl_flags;
	u8 simple_sgl_flags_last;
	u8 chain_sgl_flags;
	struct leapioraid_chain_tracker *chain_req;

	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	simple_sgl_flags = LEAPIORAID_IEEE_SGE_FLAGS_SIMPLE_ELEMENT |
	    LEAPIORAID_IEEE_SGE_FLAGS_SYSTEM_ADDR;
	simple_sgl_flags_last = simple_sgl_flags |
	    LEAPIORAID_IEEE_SGE_FLAGS_END_OF_LIST;
	chain_sgl_flags = LEAPIORAID_IEEE_SGE_FLAGS_CHAIN_ELEMENT |
	    LEAPIORAID_IEEE_SGE_FLAGS_SYSTEM_ADDR;

	sg_scmd = scsi_sglist(scmd);
	sges_left = scsi_dma_map(scmd);
	if (sges_left < 0) {
		pr_err_ratelimited
		    ("sd %s: scsi_dma_map failed: request for %d bytes!\n",
		     dev_name(&scmd->device->sdev_gendev), scsi_bufflen(scmd));
		return -ENOMEM;
	}
	sg_local = &mpi_request->SGL;
	sges_in_segment = (ioc->request_sz -
			   offsetof(struct LeapioraidSCSIIOReq_t,
				    SGL)) / ioc->sge_size_ieee;
	if (sges_left <= sges_in_segment)
		goto fill_in_last_segment;
	mpi_request->ChainOffset = (sges_in_segment - 1) +
	    (offsetof(struct LeapioraidSCSIIOReq_t, SGL) / ioc->sge_size_ieee);
	while (sges_in_segment > 1) {
		leapioraid_base_add_sg_single_ieee(sg_local, simple_sgl_flags,
						   0, sg_dma_len(sg_scmd),
						   sg_dma_address(sg_scmd));

		sg_scmd = sg_next(sg_scmd);
		sg_local += ioc->sge_size_ieee;
		sges_left--;
		sges_in_segment--;
	}
	chain_req = leapioraid_base_get_chain_buffer_tracker(ioc, scmd);
	if (!chain_req)
		return -1;
	chain = chain_req->chain_buffer;
	chain_dma = chain_req->chain_buffer_dma;
	do {
		sges_in_segment = (sges_left <=
				   ioc->max_sges_in_chain_message) ? sges_left :
		    ioc->max_sges_in_chain_message;
		chain_offset = (sges_left == sges_in_segment) ?
		    0 : sges_in_segment;
		chain_length = sges_in_segment * ioc->sge_size_ieee;
		if (chain_offset)
			chain_length += ioc->sge_size_ieee;
		leapioraid_base_add_sg_single_ieee(sg_local, chain_sgl_flags,
						   chain_offset, chain_length,
						   chain_dma);
		sg_local = chain;
		if (!chain_offset)
			goto fill_in_last_segment;
		while (sges_in_segment) {
			leapioraid_base_add_sg_single_ieee(sg_local,
							   simple_sgl_flags, 0,
							   sg_dma_len(sg_scmd),
							   sg_dma_address
							   (sg_scmd));

			sg_scmd = sg_next(sg_scmd);
			sg_local += ioc->sge_size_ieee;
			sges_left--;
			sges_in_segment--;
		}
		chain_req = leapioraid_base_get_chain_buffer_tracker(ioc, scmd);
		if (!chain_req)
			return -1;
		chain = chain_req->chain_buffer;
		chain_dma = chain_req->chain_buffer_dma;
	} while (1);
fill_in_last_segment:
	while (sges_left > 0) {
		if (sges_left == 1)
			leapioraid_base_add_sg_single_ieee(sg_local,
							   simple_sgl_flags_last,
							   0,
							   sg_dma_len(sg_scmd),
							   sg_dma_address
							   (sg_scmd));
		else
			leapioraid_base_add_sg_single_ieee(sg_local,
							   simple_sgl_flags, 0,
							   sg_dma_len(sg_scmd),
							   sg_dma_address
							   (sg_scmd));

		sg_scmd = sg_next(sg_scmd);
		sg_local += ioc->sge_size_ieee;
		sges_left--;
	}
	return 0;
}

static void
leapioraid_base_build_sg_ieee(struct LEAPIORAID_ADAPTER *ioc, void *psge,
			      dma_addr_t data_out_dma, size_t data_out_sz,
			      dma_addr_t data_in_dma, size_t data_in_sz)
{
	u8 sgl_flags;

	if (!data_out_sz && !data_in_sz) {
		leapioraid_base_build_zero_len_sge_ieee(ioc, psge);
		return;
	}
	if (data_out_sz && data_in_sz) {
		sgl_flags = LEAPIORAID_IEEE_SGE_FLAGS_SIMPLE_ELEMENT |
		    LEAPIORAID_IEEE_SGE_FLAGS_SYSTEM_ADDR;
		leapioraid_base_add_sg_single_ieee(psge, sgl_flags, 0,
						   data_out_sz, data_out_dma);
		psge += ioc->sge_size_ieee;
		sgl_flags |= LEAPIORAID_IEEE_SGE_FLAGS_END_OF_LIST;
		leapioraid_base_add_sg_single_ieee(psge, sgl_flags, 0,
						   data_in_sz, data_in_dma);
	} else if (data_out_sz) {
		sgl_flags = LEAPIORAID_IEEE_SGE_FLAGS_SIMPLE_ELEMENT |
		    LEAPIORAID_IEEE_SGE_FLAGS_END_OF_LIST |
		    LEAPIORAID_IEEE_SGE_FLAGS_SYSTEM_ADDR;
		leapioraid_base_add_sg_single_ieee(psge, sgl_flags, 0,
						   data_out_sz, data_out_dma);
	} else if (data_in_sz) {
		sgl_flags = LEAPIORAID_IEEE_SGE_FLAGS_SIMPLE_ELEMENT |
		    LEAPIORAID_IEEE_SGE_FLAGS_END_OF_LIST |
		    LEAPIORAID_IEEE_SGE_FLAGS_SYSTEM_ADDR;
		leapioraid_base_add_sg_single_ieee(psge, sgl_flags, 0,
						   data_in_sz, data_in_dma);
	}
}

#define leapioraid_convert_to_kb(x) ((x) << (PAGE_SHIFT - 10))
static int
leapioraid_base_config_dma_addressing(struct LEAPIORAID_ADAPTER *ioc,
				      struct pci_dev *pdev)
{
	struct sysinfo s;
	char *desc = "64";
	u64 consistant_dma_mask = DMA_BIT_MASK(64);
	u64 dma_mask = DMA_BIT_MASK(64);

	consistant_dma_mask = DMA_BIT_MASK(63);
	dma_mask = DMA_BIT_MASK(63);
	desc = "63";
	ioc->dma_mask = 63;
	if (ioc->use_32bit_dma)
		consistant_dma_mask = DMA_BIT_MASK(32);
	if (sizeof(dma_addr_t) > 4) {
		if (!dma_set_mask(&pdev->dev, dma_mask) &&
		    !dma_set_coherent_mask(&pdev->dev, consistant_dma_mask)) {
			ioc->base_add_sg_single =
			    &leapioraid_base_add_sg_single_64;
			ioc->sge_size = sizeof(struct LeapioSGESimple64_t);
			if (!ioc->use_32bit_dma)
				goto out;
			return 0;
		}
	}
	if (!dma_set_mask(&pdev->dev, DMA_BIT_MASK(32))
	    && !dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32))) {
		ioc->base_add_sg_single = &leapioraid_base_add_sg_single_32;
		ioc->sge_size = sizeof(struct LeapioSGESimple32_t);
		desc = "32";
		ioc->dma_mask = 32;
	} else
		return -ENODEV;
out:
	si_meminfo(&s);
	pr_info("%s %s BIT PCI BUS DMA ADDRESSING SUPPORTED, total mem (%ld kB)\n",
		ioc->name, desc, leapioraid_convert_to_kb(s.totalram));
	return 0;
}

int
leapioraid_base_check_and_get_msix_vectors(struct pci_dev *pdev)
{
	int base;
	u16 message_control, msix_vector_count;

	base = pci_find_capability(pdev, PCI_CAP_ID_MSIX);
	if (!base)
		return -EINVAL;
	pci_read_config_word(pdev, base + 2, &message_control);
	msix_vector_count = (message_control & 0x3FF) + 1;
	return msix_vector_count;
}

enum leapioraid_pci_bus_speed {
	LEAPIORAID_PCIE_SPEED_2_5GT = 0x14,
	LEAPIORAID_PCIE_SPEED_5_0GT = 0x15,
	LEAPIORAID_PCIE_SPEED_8_0GT = 0x16,
	LEAPIORAID_PCIE_SPEED_16_0GT = 0x17,
	LEAPIORAID_PCI_SPEED_UNKNOWN = 0xff,
};

const unsigned char leapioraid_pcie_link_speed[] = {
	LEAPIORAID_PCI_SPEED_UNKNOWN,
	LEAPIORAID_PCIE_SPEED_2_5GT,
	LEAPIORAID_PCIE_SPEED_5_0GT,
	LEAPIORAID_PCIE_SPEED_8_0GT,
	LEAPIORAID_PCIE_SPEED_16_0GT,
	LEAPIORAID_PCI_SPEED_UNKNOWN,
	LEAPIORAID_PCI_SPEED_UNKNOWN,
	LEAPIORAID_PCI_SPEED_UNKNOWN,
	LEAPIORAID_PCI_SPEED_UNKNOWN,
	LEAPIORAID_PCI_SPEED_UNKNOWN,
	LEAPIORAID_PCI_SPEED_UNKNOWN,
	LEAPIORAID_PCI_SPEED_UNKNOWN,
	LEAPIORAID_PCI_SPEED_UNKNOWN,
	LEAPIORAID_PCI_SPEED_UNKNOWN,
	LEAPIORAID_PCI_SPEED_UNKNOWN,
	LEAPIORAID_PCI_SPEED_UNKNOWN
};

static void
leapioraid_base_check_and_enable_high_iops_queues(
	struct LEAPIORAID_ADAPTER *ioc,
	int hba_msix_vector_count,
	int iopoll_q_count)
{
	u16 lnksta;
	enum leapioraid_pci_bus_speed speed;

	if (perf_mode == LEAPIORAID_PERF_MODE_IOPS ||
	    perf_mode == LEAPIORAID_PERF_MODE_LATENCY || iopoll_q_count) {
		ioc->high_iops_queues = 0;
		return;
	}
	if (perf_mode == LEAPIORAID_PERF_MODE_DEFAULT) {
		pcie_capability_read_word(ioc->pdev, PCI_EXP_LNKSTA, &lnksta);
		speed = leapioraid_pcie_link_speed[lnksta & PCI_EXP_LNKSTA_CLS];
		dev_info(&ioc->pdev->dev, "PCIe device speed is %s\n",
			 speed == LEAPIORAID_PCIE_SPEED_2_5GT ? "2.5GHz" :
			 speed == LEAPIORAID_PCIE_SPEED_5_0GT ? "5.0GHz" :
			 speed == LEAPIORAID_PCIE_SPEED_8_0GT ? "8.0GHz" :
			 speed == LEAPIORAID_PCIE_SPEED_16_0GT ? "16.0GHz" :
			 "Unknown");
		if (speed < LEAPIORAID_PCIE_SPEED_16_0GT) {
			ioc->high_iops_queues = 0;
			return;
		}
	}
	if (!reset_devices &&
	    hba_msix_vector_count == LEAPIORAID_GEN35_MAX_MSIX_QUEUES &&
	    num_online_cpus() >= LEAPIORAID_HIGH_IOPS_REPLY_QUEUES &&
	    max_msix_vectors == -1)
		ioc->high_iops_queues = LEAPIORAID_HIGH_IOPS_REPLY_QUEUES;
	else
		ioc->high_iops_queues = 0;
}

void
leapioraid_base_disable_msix(struct LEAPIORAID_ADAPTER *ioc)
{
	if (!ioc->msix_enable)
		return;
	pci_free_irq_vectors(ioc->pdev);
	kfree(ioc->blk_mq_poll_queues);
	ioc->msix_enable = 0;
}

void
leapioraid_base_free_irq(struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_adapter_reply_queue *reply_q, *next;

	if (list_empty(&ioc->reply_queue_list))
		return;
	list_for_each_entry_safe(reply_q, next, &ioc->reply_queue_list, list) {
		list_del(&reply_q->list);
		if (reply_q->is_blk_mq_poll_q) {
			kfree(reply_q);
			continue;
		}
		irq_poll_disable(&reply_q->irqpoll);
		if (ioc->smp_affinity_enable)
			irq_set_affinity_hint(pci_irq_vector(ioc->pdev,
							     reply_q->msix_index), NULL);
		free_irq(pci_irq_vector(ioc->pdev, reply_q->msix_index),
			 reply_q);
		kfree(reply_q);
	}
}

static int
leapioraid_base_request_irq(struct LEAPIORAID_ADAPTER *ioc, u8 index)
{
	struct leapioraid_adapter_reply_queue *reply_q;
	int r;
	u8 qid;

	reply_q = kzalloc(sizeof(struct leapioraid_adapter_reply_queue),
		GFP_KERNEL);
	if (!reply_q)
		return -ENOMEM;

	reply_q->ioc = ioc;
	reply_q->msix_index = index;
	atomic_set(&reply_q->busy, 0);
	if (index >= ioc->iopoll_q_start_index) {
		qid = index - ioc->iopoll_q_start_index;
		snprintf(reply_q->name, LEAPIORAID_NAME_LENGTH, "%s%u-mq-poll%u",
			 ioc->driver_name, ioc->id, qid);
		reply_q->is_blk_mq_poll_q = 1;
		ioc->blk_mq_poll_queues[qid].reply_q = reply_q;
		INIT_LIST_HEAD(&reply_q->list);
		list_add_tail(&reply_q->list, &ioc->reply_queue_list);
		return 0;
	}
	if (ioc->msix_enable)
		snprintf(reply_q->name, LEAPIORAID_NAME_LENGTH, "%s%u-msix%u",
			 ioc->driver_name, ioc->id, index);
	else
		snprintf(reply_q->name, LEAPIORAID_NAME_LENGTH, "%s%d",
			 ioc->driver_name, ioc->id);
	r = request_irq(pci_irq_vector(ioc->pdev, index), leapioraid_base_interrupt,
			IRQF_SHARED, reply_q->name, reply_q);
	if (r) {
		pr_err("%s unable to allocate interrupt %d!\n", reply_q->name,
		       pci_irq_vector(ioc->pdev, index));
		kfree(reply_q);
		return -EBUSY;
	}

	INIT_LIST_HEAD(&reply_q->list);
	list_add_tail(&reply_q->list, &ioc->reply_queue_list);
	return 0;
}

static int leapioraid_base_alloc_irq_vectors(struct LEAPIORAID_ADAPTER *ioc)
{
	int i, irq_flags = PCI_IRQ_MSIX;
	struct irq_affinity desc = {.pre_vectors = ioc->high_iops_queues };
	struct irq_affinity *descp = &desc;
	int nr_msix_vectors = ioc->iopoll_q_start_index;

	if (ioc->smp_affinity_enable)
		irq_flags |= PCI_IRQ_AFFINITY | PCI_IRQ_ALL_TYPES;
	else
		descp = NULL;
	dinitprintk(ioc, pr_err(
		"%s high_iops_queues: %d,\n\t\t"
			"reply_queue_count: %d, nr_msix_vectors: %d\n",
				ioc->name,
				ioc->high_iops_queues,
				ioc->reply_queue_count,
				nr_msix_vectors));
	i = pci_alloc_irq_vectors_affinity(
		ioc->pdev,
		ioc->high_iops_queues,
		nr_msix_vectors, irq_flags, descp);
	return i;
}

static int
leapioraid_base_enable_msix(struct LEAPIORAID_ADAPTER *ioc)
{
	int r, i, msix_vector_count, local_max_msix_vectors;
	int iopoll_q_count = 0;

	ioc->msix_load_balance = false;
	msix_vector_count =
	    leapioraid_base_check_and_get_msix_vectors(ioc->pdev);
	if (msix_vector_count <= 0) {
		dfailprintk(ioc, pr_info("%s msix not supported\n", ioc->name));
		goto try_ioapic;
	}
	dinitprintk(ioc, pr_err(
				"%s MSI-X vectors supported: %d, no of cores: %d\n",
				ioc->name, msix_vector_count, ioc->cpu_count));
	ioc->reply_queue_count = min_t(int, ioc->cpu_count, msix_vector_count);
	if (!ioc->rdpq_array_enable && max_msix_vectors == -1) {
		if (reset_devices)
			local_max_msix_vectors = 1;
		else
			local_max_msix_vectors = 8;
	} else
		local_max_msix_vectors = max_msix_vectors;
	if (local_max_msix_vectors == 0)
		goto try_ioapic;
	if (!ioc->combined_reply_queue) {
		pr_err(
			"%s combined reply queue is off, so enabling msix load balance\n",
		    ioc->name);
		ioc->msix_load_balance = true;
	}
	if (ioc->msix_load_balance)
		ioc->smp_affinity_enable = 0;
	if (!ioc->smp_affinity_enable || ioc->reply_queue_count <= 1)
		ioc->shost->host_tagset = 0;
	if (ioc->shost->host_tagset)
		iopoll_q_count = poll_queues;
	if (iopoll_q_count) {
		ioc->blk_mq_poll_queues = kcalloc(iopoll_q_count,
						  sizeof(struct
							 leapioraid_blk_mq_poll_queue),
						  GFP_KERNEL);
		if (!ioc->blk_mq_poll_queues)
			iopoll_q_count = 0;
	}
	leapioraid_base_check_and_enable_high_iops_queues(ioc,
								msix_vector_count,
								iopoll_q_count);
	ioc->reply_queue_count =
	    min_t(int, ioc->reply_queue_count + ioc->high_iops_queues,
		  msix_vector_count);
	if (local_max_msix_vectors > 0)
		ioc->reply_queue_count = min_t(int, local_max_msix_vectors,
					       ioc->reply_queue_count);
	if (iopoll_q_count) {
		if (ioc->reply_queue_count < (iopoll_q_count + 1))
			iopoll_q_count = 0;
		ioc->reply_queue_count =
		    min(ioc->reply_queue_count + iopoll_q_count,
			msix_vector_count);
	}
	ioc->iopoll_q_start_index = ioc->reply_queue_count - iopoll_q_count;
	r = leapioraid_base_alloc_irq_vectors(ioc);
	if (r < 0) {
		pr_warn(
		       "%s pci_alloc_irq_vectors failed (r=%d) !!!\n",
		       ioc->name, r);
		goto try_ioapic;
	}
	ioc->msix_enable = 1;
	for (i = 0; i < ioc->reply_queue_count; i++) {
		r = leapioraid_base_request_irq(ioc, i);
		if (r) {
			leapioraid_base_free_irq(ioc);
			leapioraid_base_disable_msix(ioc);
			goto try_ioapic;
		}
	}
	dinitprintk(ioc,
		    pr_info("%s High IOPs queues : %s\n",
			   ioc->name,
			   ioc->high_iops_queues ? "enabled" : "disabled"));
	return 0;
try_ioapic:
	ioc->high_iops_queues = 0;
	dinitprintk(ioc, pr_err(
				"%s High IOPs queues : disabled\n", ioc->name));
	ioc->reply_queue_count = 1;
	ioc->iopoll_q_start_index = ioc->reply_queue_count - 0;
	r = leapioraid_base_request_irq(ioc, 0);
	return r;
}

static void
leapioraid_base_import_managed_irqs_affinity(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_adapter_reply_queue *reply_q;
	unsigned int cpu, nr_msix;
	int local_numa_node;
	unsigned int index = 0;

	nr_msix = ioc->reply_queue_count;
	if (!nr_msix)
		return;
	if (ioc->smp_affinity_enable) {
		if (ioc->high_iops_queues) {
			local_numa_node = dev_to_node(&ioc->pdev->dev);
			for (index = 0; index < ioc->high_iops_queues; index++) {
				irq_set_affinity_hint(pci_irq_vector(ioc->pdev,
								     index),
						      cpumask_of_node
						      (local_numa_node));
			}
		}
		list_for_each_entry(reply_q, &ioc->reply_queue_list, list) {
			const cpumask_t *mask;

			if (reply_q->msix_index < ioc->high_iops_queues ||
			    reply_q->msix_index >= ioc->iopoll_q_start_index)
				continue;
			mask = pci_irq_get_affinity(ioc->pdev,
						    reply_q->msix_index);
			if (!mask) {
				dinitprintk(ioc, pr_warn(
							"%s no affinity for msi %x\n",
							ioc->name,
							reply_q->msix_index));
				goto fall_back;
			}
			for_each_cpu_and(cpu, mask, cpu_online_mask) {
				if (cpu >= ioc->cpu_msix_table_sz)
					break;
				ioc->cpu_msix_table[cpu] = reply_q->msix_index;
			}
		}
		return;
	}
fall_back:
	leapioraid_base_group_cpus_on_irq(ioc);
}

static void
leapioraid_base_assign_reply_queues(struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_adapter_reply_queue *reply_q;
	int reply_queue;

	if (!leapioraid_base_is_controller_msix_enabled(ioc))
		return;
	if (ioc->msix_load_balance)
		return;
	memset(ioc->cpu_msix_table, 0, ioc->cpu_msix_table_sz);
	if (ioc->reply_queue_count > ioc->facts.MaxMSIxVectors) {
		ioc->reply_queue_count = ioc->facts.MaxMSIxVectors;
		reply_queue = 0;
		list_for_each_entry(reply_q, &ioc->reply_queue_list, list) {
			reply_q->msix_index = reply_queue;
			if (++reply_queue == ioc->reply_queue_count)
				reply_queue = 0;
		}
	}
	leapioraid_base_import_managed_irqs_affinity(ioc);
}

static int
leapioraid_base_wait_for_doorbell_int(
	struct LEAPIORAID_ADAPTER *ioc, int timeout)
{
	u32 cntdn, count;
	u32 int_status;

	count = 0;
	cntdn = 1000 * timeout;
	do {
		int_status =
		    ioc->base_readl(&ioc->chip->HostInterruptStatus,
				    LEAPIORAID_READL_RETRY_COUNT_OF_THREE);
		if (int_status & LEAPIORAID_HIS_IOC2SYS_DB_STATUS) {
			dhsprintk(ioc, pr_info(
				"%s %s: successful count(%d), timeout(%d)\n",
				ioc->name, __func__, count,
				timeout));
			return 0;
		}
		usleep_range(1000, 1100);
		count++;
	} while (--cntdn);
	pr_err("%s %s: failed due to timeout count(%d), int_status(%x)!\n",
		ioc->name, __func__, count, int_status);
	return -EFAULT;
}

static int
leapioraid_base_spin_on_doorbell_int(struct LEAPIORAID_ADAPTER *ioc,
				     int timeout)
{
	u32 cntdn, count;
	u32 int_status;

	count = 0;
	cntdn = 2000 * timeout;
	do {
		int_status =
		    ioc->base_readl(&ioc->chip->HostInterruptStatus,
				    LEAPIORAID_READL_RETRY_COUNT_OF_THREE);
		if (int_status & LEAPIORAID_HIS_IOC2SYS_DB_STATUS) {
			dhsprintk(ioc, pr_info(
					       "%s %s: successful count(%d), timeout(%d)\n",
					       ioc->name, __func__, count,
					       timeout));
			return 0;
		}
		udelay(500);
		count++;
	} while (--cntdn);
	pr_err("%s %s: failed due to timeout count(%d), int_status(%x)!\n",
		ioc->name, __func__, count, int_status);
	return -EFAULT;
}

static int
leapioraid_base_wait_for_doorbell_ack(struct LEAPIORAID_ADAPTER *ioc,
				      int timeout)
{
	u32 cntdn, count;
	u32 int_status;
	u32 doorbell;

	count = 0;
	cntdn = 1000 * timeout;
	do {
		int_status =
		    ioc->base_readl(&ioc->chip->HostInterruptStatus,
				    LEAPIORAID_READL_RETRY_COUNT_OF_THREE);
		if (!(int_status & LEAPIORAID_HIS_SYS2IOC_DB_STATUS)) {
			dhsprintk(ioc, pr_info(
				"%s %s: successful count(%d), timeout(%d)\n",
				ioc->name, __func__, count,
				timeout));
			return 0;
		} else if (int_status & LEAPIORAID_HIS_IOC2SYS_DB_STATUS) {
			doorbell =
			    ioc->base_readl(&ioc->chip->Doorbell,
					    LEAPIORAID_READL_RETRY_COUNT_OF_THIRTY);
			if ((doorbell & LEAPIORAID_IOC_STATE_MASK) ==
			    LEAPIORAID_IOC_STATE_FAULT) {
				leapioraid_print_fault_code(ioc, doorbell);
				return -EFAULT;
			}
			if ((doorbell & LEAPIORAID_IOC_STATE_MASK) ==
			    LEAPIORAID_IOC_STATE_COREDUMP) {
				leapioraid_base_coredump_info(ioc, doorbell);
				return -EFAULT;
			}
		} else if (int_status == 0xFFFFFFFF)
			goto out;
		usleep_range(1000, 1100);
		count++;
	} while (--cntdn);
out:
	pr_err("%s %s: failed due to timeout count(%d), int_status(%x)!\n",
		ioc->name, __func__, count, int_status);
	return -EFAULT;
}

static int
leapioraid_base_wait_for_doorbell_not_used(struct LEAPIORAID_ADAPTER *ioc,
					   int timeout)
{
	u32 cntdn, count;
	u32 doorbell_reg;

	count = 0;
	cntdn = 1000 * timeout;
	do {
		doorbell_reg =
		    ioc->base_readl(&ioc->chip->Doorbell,
				    LEAPIORAID_READL_RETRY_COUNT_OF_THIRTY);
		if (!(doorbell_reg & LEAPIORAID_DOORBELL_USED)) {
			dhsprintk(ioc, pr_info(
				"%s %s: successful count(%d), timeout(%d)\n",
				ioc->name, __func__, count,
				timeout));
			return 0;
		}
		usleep_range(1000, 1100);
		count++;
	} while (--cntdn);
	pr_err("%s %s: failed due to timeout count(%d), doorbell_reg(%x)!\n",
		ioc->name, __func__, count, doorbell_reg);
	return -EFAULT;
}

static int
leapioraid_base_handshake_req_reply_wait(struct LEAPIORAID_ADAPTER *ioc,
					 int request_bytes, u32 *request,
					 int reply_bytes, u16 *reply,
					 int timeout)
{
	struct LeapioraidDefaultRep_t *default_reply
		= (struct LeapioraidDefaultRep_t *) reply;
	int i;
	u8 failed;
	__le32 *mfp;

	if ((ioc->base_readl(&ioc->chip->Doorbell,
			LEAPIORAID_READL_RETRY_COUNT_OF_THIRTY) & LEAPIORAID_DOORBELL_USED)) {
		pr_err("%s doorbell is in use  (line=%d)\n", ioc->name, __LINE__);
		return -EFAULT;
	}
	if (ioc->base_readl(&ioc->chip->HostInterruptStatus,
		       LEAPIORAID_READL_RETRY_COUNT_OF_THREE) &
	    LEAPIORAID_HIS_IOC2SYS_DB_STATUS)
		writel(0, &ioc->chip->HostInterruptStatus);
	writel(((LEAPIORAID_FUNC_HANDSHAKE << LEAPIORAID_DOORBELL_FUNCTION_SHIFT)
		| ((request_bytes / 4) << LEAPIORAID_DOORBELL_ADD_DWORDS_SHIFT)),
	       &ioc->chip->Doorbell);
	if ((leapioraid_base_spin_on_doorbell_int(ioc, 5))) {
		pr_err("%s doorbell handshake int failed (line=%d)\n",
			ioc->name, __LINE__);
		return -EFAULT;
	}
	writel(0, &ioc->chip->HostInterruptStatus);
	if ((leapioraid_base_wait_for_doorbell_ack(ioc, 5))) {
		pr_err("%s doorbell handshake ack failed (line=%d)\n",
			ioc->name, __LINE__);
		return -EFAULT;
	}
	for (i = 0, failed = 0; i < request_bytes / 4 && !failed; i++) {
		writel((u32) (request[i]), &ioc->chip->Doorbell);
		if ((leapioraid_base_wait_for_doorbell_ack(ioc, 5)))
			failed = 1;
	}
	if (failed) {
		pr_err("%s doorbell handshake sending request failed (line=%d)\n",
			ioc->name, __LINE__);
		return -EFAULT;
	}
	if ((leapioraid_base_wait_for_doorbell_int(ioc, timeout))) {
		pr_err("%s doorbell handshake int failed (line=%d)\n",
			ioc->name, __LINE__);
		return -EFAULT;
	}
	reply[0] =
	    (u16) (ioc->base_readl(&ioc->chip->Doorbell,
					LEAPIORAID_READL_RETRY_COUNT_OF_THIRTY)
					& LEAPIORAID_DOORBELL_DATA_MASK);
	writel(0, &ioc->chip->HostInterruptStatus);
	if ((leapioraid_base_wait_for_doorbell_int(ioc, 5))) {
		pr_err("%s doorbell handshake int failed (line=%d)\n",
			ioc->name, __LINE__);
		return -EFAULT;
	}
	reply[1] =
	    (u16) (ioc->base_readl(&ioc->chip->Doorbell,
			LEAPIORAID_READL_RETRY_COUNT_OF_THIRTY)
			& LEAPIORAID_DOORBELL_DATA_MASK);
	writel(0, &ioc->chip->HostInterruptStatus);
	for (i = 2; i < default_reply->MsgLength * 2; i++) {
		if ((leapioraid_base_wait_for_doorbell_int(ioc, 5))) {
			pr_err("%s doorbell handshake int failed (line=%d)\n",
				ioc->name, __LINE__);
			return -EFAULT;
		}
		if (i >= reply_bytes / 2)
			ioc->base_readl(&ioc->chip->Doorbell,
					LEAPIORAID_READL_RETRY_COUNT_OF_THIRTY);
		else
			reply[i] =
			    (u16) (ioc->base_readl(&ioc->chip->Doorbell,
					      LEAPIORAID_READL_RETRY_COUNT_OF_THIRTY)
						& LEAPIORAID_DOORBELL_DATA_MASK);
		writel(0, &ioc->chip->HostInterruptStatus);
	}
	if (leapioraid_base_wait_for_doorbell_int(ioc, 5)) {
		pr_err("%s doorbell handshake int failed (line=%d)\n",
			ioc->name, __LINE__);
		return -EFAULT;
	}
	if (leapioraid_base_wait_for_doorbell_not_used(ioc, 5) != 0) {
		dhsprintk(ioc,
			pr_info("%s doorbell is in use (line=%d)\n",
				ioc->name, __LINE__));
	}
	writel(0, &ioc->chip->HostInterruptStatus);
	if (ioc->logging_level & LEAPIORAID_DEBUG_INIT) {
		mfp = (__le32 *) reply;
		pr_info("%s \toffset:data\n", ioc->name);
		for (i = 0; i < reply_bytes / 4; i++)
			pr_info("%s \t[0x%02x]:%08x\n",
				ioc->name, i * 4, le32_to_cpu(mfp[i]));
	}
	return 0;
}

static int
leapioraid_base_wait_on_iocstate(
	struct LEAPIORAID_ADAPTER *ioc, u32 ioc_state,
	int timeout)
{
	u32 count, cntdn;
	u32 current_state;

	count = 0;
	cntdn = 1000 * timeout;
	do {
		current_state = leapioraid_base_get_iocstate(ioc, 1);
		if (current_state == ioc_state)
			return 0;
		if (count && current_state == LEAPIORAID_IOC_STATE_FAULT)
			break;
		usleep_range(1000, 1100);
		count++;
	} while (--cntdn);
	return current_state;
}

static inline void
leapioraid_base_dump_reg_set(struct LEAPIORAID_ADAPTER *ioc)
{
	unsigned int i, sz = 256;
	u32 __iomem *reg = (u32 __iomem *) ioc->chip;

	pr_info("%s System Register set:\n", ioc->name);
	for (i = 0; i < (sz / sizeof(u32)); i++)
		pr_info("%08x: %08x\n", (i * 4), readl(&reg[i]));
}

int
leapioraid_base_unlock_and_get_host_diagnostic(
	struct LEAPIORAID_ADAPTER *ioc,
	u32 *host_diagnostic)
{
	u32 count;

	*host_diagnostic = 0;
	count = 0;
	do {
		drsprintk(ioc, pr_info("%s write magic sequence\n", ioc->name));
		writel(0x0, &ioc->chip->WriteSequence);
		writel(0xF, &ioc->chip->WriteSequence);
		writel(0x4, &ioc->chip->WriteSequence);
		writel(0xB, &ioc->chip->WriteSequence);
		writel(0x2, &ioc->chip->WriteSequence);
		writel(0x7, &ioc->chip->WriteSequence);
		writel(0xD, &ioc->chip->WriteSequence);
		msleep(100);
		if (count++ > 20) {
			pr_err("%s Giving up writing magic sequence after 20 retries\n",
			       ioc->name);
			leapioraid_base_dump_reg_set(ioc);
			return -EFAULT;
		}
		*host_diagnostic =
			ioc->base_readl(&ioc->chip->HostDiagnostic,
				LEAPIORAID_READL_RETRY_COUNT_OF_THIRTY);
		drsprintk(ioc, pr_info(
			"%s wrote magic sequence: cnt(%d), host_diagnostic(0x%08x)\n",
			ioc->name, count, *host_diagnostic));
	} while ((*host_diagnostic & 0x00000080) == 0);
	return 0;
}

void
leapioraid_base_lock_host_diagnostic(struct LEAPIORAID_ADAPTER *ioc)
{
	drsprintk(ioc, pr_info("%s disable writes to the diagnostic register\n",
		ioc->name));
	writel(0x0, &ioc->chip->WriteSequence);
}

static int
leapioraid_base_diag_reset(struct LEAPIORAID_ADAPTER *ioc)
{
	u32 host_diagnostic;
	u32 ioc_state;
	u32 count;
	u32 hcb_size;

	pr_info("%s sending diag reset !!\n", ioc->name);
	drsprintk(ioc,
		  pr_info("%s Locking pci cfg space access\n",
			 ioc->name));
	pci_cfg_access_lock(ioc->pdev);
	drsprintk(ioc, pr_info("%s clear interrupts\n",
			      ioc->name));
	mutex_lock(&ioc->hostdiag_unlock_mutex);
	if (leapioraid_base_unlock_and_get_host_diagnostic
	    (ioc, &host_diagnostic)) {
		mutex_unlock(&ioc->hostdiag_unlock_mutex);
		goto out;
	}
	hcb_size =
	    ioc->base_readl(&ioc->chip->HCBSize, LEAPIORAID_READL_RETRY_COUNT_OF_THREE);
	drsprintk(ioc,
		  pr_info("%s diag reset: issued\n",
			 ioc->name));
	writel(host_diagnostic | LEAPIORAID_DIAG_RESET_ADAPTER,
	       &ioc->chip->HostDiagnostic);
#if defined(DISABLE_RESET_SUPPORT)
	count = 0;
	do {
		msleep(50);
		host_diagnostic =
		    ioc->base_readl(&ioc->chip->HostDiagnostic,
				    LEAPIORAID_READL_RETRY_COUNT_OF_THIRTY);
		if (host_diagnostic == 0xFFFFFFFF)
			goto out;
		else if (count++ >= 300)
			goto out;
		if (!(count % 20))
			pr_info("waiting on diag reset bit to clear, count = %d\n",
				(count / 20));
	} while (host_diagnostic & LEAPIORAID_DIAG_RESET_ADAPTER);
#else
	msleep(50);
	for (count = 0; count < (300000 / 256); count++) {
		host_diagnostic =
		    ioc->base_readl(&ioc->chip->HostDiagnostic,
				    LEAPIORAID_READL_RETRY_COUNT_OF_THIRTY);
		if (host_diagnostic == 0xFFFFFFFF) {
			pr_err("%s Invalid host diagnostic register value\n",
			       ioc->name);
			leapioraid_base_dump_reg_set(ioc);
			goto out;
		}
		if (!(host_diagnostic & LEAPIORAID_DIAG_RESET_ADAPTER))
			break;

		msleep(256);
	}
#endif
	if (host_diagnostic & 0x00000100) {
		drsprintk(ioc, pr_info(
			"%s restart IOC assuming HCB Address points to good F/W\n",
			ioc->name));
		host_diagnostic &= ~0x00001800;
		host_diagnostic |= 0x00000800;
		writel(host_diagnostic, &ioc->chip->HostDiagnostic);
		drsprintk(ioc, pr_err(
				      "%s re-enable the HCDW\n", ioc->name));
		writel(hcb_size | 0x00000001,
		       &ioc->chip->HCBSize);
	}
	drsprintk(ioc, pr_info("%s restart the adapter\n",
			      ioc->name));
	writel(host_diagnostic & ~0x00000002,
	       &ioc->chip->HostDiagnostic);
	leapioraid_base_lock_host_diagnostic(ioc);
	mutex_unlock(&ioc->hostdiag_unlock_mutex);
	drsprintk(ioc, pr_info("%s Wait for FW to go to the READY state\n",
		ioc->name));
	ioc_state =
	    leapioraid_base_wait_on_iocstate(
			ioc, LEAPIORAID_IOC_STATE_READY, 20);
	if (ioc_state) {
		pr_err("%s %s: failed going to ready state (ioc_state=0x%x)\n",
			ioc->name, __func__, ioc_state);
		leapioraid_base_dump_reg_set(ioc);
		goto out;
	}
	drsprintk(ioc, pr_err(
			      "%s Unlocking pci cfg space access\n", ioc->name));
	pci_cfg_access_unlock(ioc->pdev);
	if (ioc->open_pcie_trace)
		leapioraid_base_trace_log_init(ioc);
	pr_info("%s diag reset: SUCCESS\n", ioc->name);
	return 0;
out:
	drsprintk(ioc, pr_err(
			      "%s Unlocking pci cfg space access\n", ioc->name));
	pci_cfg_access_unlock(ioc->pdev);
	pr_err("%s diag reset: FAILED\n", ioc->name);
	mutex_unlock(&ioc->hostdiag_unlock_mutex);
	return -EFAULT;
}

static int
leapioraid_base_wait_for_iocstate(
	struct LEAPIORAID_ADAPTER *ioc, int timeout)
{
	u32 ioc_state;
	int rc;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	if (!leapioraid_base_pci_device_is_available(ioc))
		return 0;
	ioc_state = leapioraid_base_get_iocstate(ioc, 0);
	dhsprintk(ioc, pr_info("%s %s: ioc_state(0x%08x)\n",
			      ioc->name, __func__, ioc_state));
	if (((ioc_state & LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_READY) ||
	    (ioc_state & LEAPIORAID_IOC_STATE_MASK) ==
	    LEAPIORAID_IOC_STATE_OPERATIONAL)
		return 0;
	if (ioc_state & LEAPIORAID_DOORBELL_USED) {
		dhsprintk(ioc,
			  pr_info("%s unexpected doorbell active!\n", ioc->name));
		goto issue_diag_reset;
	}
	if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_FAULT) {
		leapioraid_print_fault_code(ioc, ioc_state &
					    LEAPIORAID_DOORBELL_DATA_MASK);
		goto issue_diag_reset;
	} else if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) ==
		   LEAPIORAID_IOC_STATE_COREDUMP) {
		pr_err("%s %s: Skipping the diag reset here. (ioc_state=0x%x)\n",
			ioc->name, __func__, ioc_state);
		return -EFAULT;
	}
	ioc_state =
	    leapioraid_base_wait_on_iocstate(ioc, LEAPIORAID_IOC_STATE_READY,
					     timeout);
	if (ioc_state) {
		pr_err("%s %s: failed going to ready state (ioc_state=0x%x)\n",
			ioc->name, __func__, ioc_state);
		return -EFAULT;
	}
issue_diag_reset:
	rc = leapioraid_base_diag_reset(ioc);
	return rc;
}

int
leapioraid_base_check_for_fault_and_issue_reset(
	struct LEAPIORAID_ADAPTER *ioc)
{
	u32 ioc_state;
	int rc = -EFAULT;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	if (!leapioraid_base_pci_device_is_available(ioc))
		return rc;
	ioc_state = leapioraid_base_get_iocstate(ioc, 0);
	dhsprintk(ioc, pr_info("%s %s: ioc_state(0x%08x)\n",
			      ioc->name, __func__, ioc_state));
	if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_FAULT) {
		leapioraid_print_fault_code(ioc, ioc_state &
					    LEAPIORAID_DOORBELL_DATA_MASK);
		leapioraid_base_mask_interrupts(ioc);
		rc = leapioraid_base_diag_reset(ioc);
	} else if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) ==
		   LEAPIORAID_IOC_STATE_COREDUMP) {
		leapioraid_base_coredump_info(ioc,
					      ioc_state &
					      LEAPIORAID_DOORBELL_DATA_MASK);
		leapioraid_base_wait_for_coredump_completion(ioc, __func__);
		leapioraid_base_mask_interrupts(ioc);
		rc = leapioraid_base_diag_reset(ioc);
	}
	return rc;
}

static int
leapioraid_base_get_ioc_facts(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidIOCFactsReq_t mpi_request;
	struct LeapioraidIOCFactsRep_t mpi_reply;
	struct leapioraid_facts *facts;
	int mpi_reply_sz, mpi_request_sz, r;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	r = leapioraid_base_wait_for_iocstate(ioc, 10);
	if (r) {
		pr_err(
		       "%s %s: failed getting to correct state\n", ioc->name,
		       __func__);
		return r;
	}
	mpi_reply_sz = sizeof(struct LeapioraidIOCFactsRep_t);
	mpi_request_sz = sizeof(struct LeapioraidIOCFactsReq_t);
	memset(&mpi_request, 0, mpi_request_sz);
	mpi_request.Function = LEAPIORAID_FUNC_IOC_FACTS;
	r = leapioraid_base_handshake_req_reply_wait(ioc, mpi_request_sz,
						     (u32 *) &mpi_request,
						     mpi_reply_sz,
						     (u16 *) &mpi_reply, 5);
	if (r != 0) {
		pr_err("%s %s: handshake failed (r=%d)\n",
		       ioc->name, __func__, r);
		return r;
	}
	facts = &ioc->facts;
	memset(facts, 0, sizeof(struct leapioraid_facts));
	facts->MsgVersion = le16_to_cpu(mpi_reply.MsgVersion);
	facts->HeaderVersion = le16_to_cpu(mpi_reply.HeaderVersion);
	facts->IOCNumber = mpi_reply.IOCNumber;
	pr_info("%s IOC Number : %d\n", ioc->name, facts->IOCNumber);
	ioc->IOCNumber = facts->IOCNumber;
	facts->VP_ID = mpi_reply.VP_ID;
	facts->VF_ID = mpi_reply.VF_ID;
	facts->IOCExceptions = le16_to_cpu(mpi_reply.IOCExceptions);
	facts->MaxChainDepth = mpi_reply.MaxChainDepth;
	facts->WhoInit = mpi_reply.WhoInit;
	facts->NumberOfPorts = mpi_reply.NumberOfPorts;
	facts->MaxMSIxVectors = mpi_reply.MaxMSIxVectors;
	if (ioc->msix_enable && (facts->MaxMSIxVectors <= 16))
		ioc->combined_reply_queue = 0;
	facts->RequestCredit = le16_to_cpu(mpi_reply.RequestCredit);
	facts->MaxReplyDescriptorPostQueueDepth =
	    le16_to_cpu(mpi_reply.MaxReplyDescriptorPostQueueDepth);
	facts->ProductID = le16_to_cpu(mpi_reply.ProductID);
	facts->IOCCapabilities = le32_to_cpu(mpi_reply.IOCCapabilities);
	if ((facts->IOCCapabilities & LEAPIORAID_IOCFACTS_CAPABILITY_INTEGRATED_RAID))
		ioc->ir_firmware = 1;
	if ((facts->IOCCapabilities & LEAPIORAID_IOCFACTS_CAPABILITY_RDPQ_ARRAY_CAPABLE)
	    && (!reset_devices))
		ioc->rdpq_array_capable = 1;
	else
		ioc->rdpq_array_capable = 0;
	if (facts->IOCCapabilities & LEAPIORAID_IOCFACTS_CAPABILITY_ATOMIC_REQ)
		ioc->atomic_desc_capable = 1;
	else
		ioc->atomic_desc_capable = 0;

	facts->FWVersion.Word = le32_to_cpu(mpi_reply.FWVersion.Word);
	facts->IOCRequestFrameSize = le16_to_cpu(mpi_reply.IOCRequestFrameSize);
	facts->IOCMaxChainSegmentSize =
	    le16_to_cpu(mpi_reply.IOCMaxChainSegmentSize);
	facts->MaxInitiators = le16_to_cpu(mpi_reply.MaxInitiators);
	facts->MaxTargets = le16_to_cpu(mpi_reply.MaxTargets);
	ioc->shost->max_id = -1;
	facts->MaxSasExpanders = le16_to_cpu(mpi_reply.MaxSasExpanders);
	facts->MaxEnclosures = le16_to_cpu(mpi_reply.MaxEnclosures);
	facts->ProtocolFlags = le16_to_cpu(mpi_reply.ProtocolFlags);
	facts->HighPriorityCredit = le16_to_cpu(mpi_reply.HighPriorityCredit);
	facts->ReplyFrameSize = mpi_reply.ReplyFrameSize;
	facts->MaxDevHandle = le16_to_cpu(mpi_reply.MaxDevHandle);
	facts->CurrentHostPageSize = mpi_reply.CurrentHostPageSize;
	ioc->page_size = 1 << facts->CurrentHostPageSize;
	if (ioc->page_size == 1) {
		pr_err(
			"%s CurrentHostPageSize is 0: Setting host page to 4k\n",
				ioc->name);
		ioc->page_size = 1 << 12;
	}
	dinitprintk(ioc,
		    pr_info("%s CurrentHostPageSize(%d)\n",
			   ioc->name, facts->CurrentHostPageSize));
	dinitprintk(ioc,
		    pr_info("%s hba queue depth(%d), max chains per io(%d)\n",
				ioc->name, facts->RequestCredit, facts->MaxChainDepth));
	dinitprintk(ioc,
		    pr_info("%s request frame size(%d), reply frame size(%d)\n",
				ioc->name,
				facts->IOCRequestFrameSize * 4,
				facts->ReplyFrameSize * 4));
	return 0;
}

static void
leapioraid_base_unmap_resources(struct LEAPIORAID_ADAPTER *ioc)
{
	struct pci_dev *pdev = ioc->pdev;

	pr_info("%s %s\n", ioc->name, __func__);
	leapioraid_base_free_irq(ioc);
	leapioraid_base_disable_msix(ioc);
	kfree(ioc->replyPostRegisterIndex);
	mutex_lock(&ioc->pci_access_mutex);
	if (ioc->chip_phys) {
		iounmap(ioc->chip);
		ioc->chip_phys = 0;
	}

	pci_release_selected_regions(ioc->pdev, ioc->bars);
	pci_disable_device(pdev);
	mutex_unlock(&ioc->pci_access_mutex);
}

int
leapioraid_base_map_resources(struct LEAPIORAID_ADAPTER *ioc)
{
	struct pci_dev *pdev = ioc->pdev;
	u32 memap_sz;
	u32 pio_sz;
	int i, r = 0, rc;
	u64 pio_chip = 0;
	phys_addr_t chip_phys = 0;
	struct leapioraid_adapter_reply_queue *reply_q;
	int iopoll_q_count = 0;

	dinitprintk(ioc, pr_info("%s %s\n",
				ioc->name, __func__));

	ioc->bars = pci_select_bars(pdev, IORESOURCE_MEM);
	if (pci_enable_device_mem(pdev)) {
		pr_warn("%s pci_enable_device_mem: failed\n", ioc->name);
		return -ENODEV;
	}
	if (pci_request_selected_regions(pdev, ioc->bars, ioc->driver_name)) {
		pr_warn("%s pci_request_selected_regions: failed\n", ioc->name);
		r = -ENODEV;
		goto out_fail;
	}

	pci_set_master(pdev);

	if (leapioraid_base_config_dma_addressing(ioc, pdev) != 0) {
		pr_warn("%s no suitable DMA mask for %s\n",
		       ioc->name, pci_name(pdev));
		r = -ENODEV;
		goto out_fail;
	}
	for (i = 0, memap_sz = 0, pio_sz = 0; i < DEVICE_COUNT_RESOURCE; i++) {
		if (pci_resource_flags(pdev, i) & IORESOURCE_IO) {
			if (pio_sz)
				continue;
			pio_chip = (u64) pci_resource_start(pdev, i);
			pio_sz = pci_resource_len(pdev, i);
		} else if (pci_resource_flags(pdev, i) & IORESOURCE_MEM) {
			if (memap_sz)
				continue;
			ioc->chip_phys = pci_resource_start(pdev, i);
			chip_phys = ioc->chip_phys;
			memap_sz = pci_resource_len(pdev, i);
			ioc->chip = ioremap(ioc->chip_phys, memap_sz);
			if (ioc->chip == NULL) {
				pr_err("%s unable to map adapter memory!\n",
				       ioc->name);
				r = -EINVAL;
				goto out_fail;
			}
		}
	}
	leapioraid_base_mask_interrupts(ioc);
	r = leapioraid_base_get_ioc_facts(ioc);
	if (r) {
		rc = leapioraid_base_check_for_fault_and_issue_reset(ioc);
		if (rc || (leapioraid_base_get_ioc_facts(ioc)))
			goto out_fail;
	}
	if (!ioc->rdpq_array_enable_assigned) {
		ioc->rdpq_array_enable = ioc->rdpq_array_capable;
		ioc->rdpq_array_enable_assigned = 1;
	}
	r = leapioraid_base_enable_msix(ioc);
	if (r)
		goto out_fail;
	iopoll_q_count = ioc->reply_queue_count - ioc->iopoll_q_start_index;
	for (i = 0; i < iopoll_q_count; i++) {
		atomic_set(&ioc->blk_mq_poll_queues[i].busy, 0);
		atomic_set(&ioc->blk_mq_poll_queues[i].pause, 0);
	}
	if (!ioc->is_driver_loading)
		leapioraid_base_init_irqpolls(ioc);
	if (ioc->combined_reply_queue) {
		ioc->replyPostRegisterIndex = kcalloc(ioc->nc_reply_index_count,
						      sizeof(resource_size_t *),
						      GFP_KERNEL);
		if (!ioc->replyPostRegisterIndex) {
			pr_err("%s allocation for reply Post Register Index failed!!!\n",
			       ioc->name);
			r = -ENOMEM;
			goto out_fail;
		}

		for (i = 0; i < ioc->nc_reply_index_count; i++) {
			ioc->replyPostRegisterIndex[i] = (resource_size_t *)
			    ((u8 *) &ioc->chip->Doorbell +
			     0x0000030C +
			     (i * 0x10));
		}
	}
	list_for_each_entry(reply_q, &ioc->reply_queue_list, list) {
		if (reply_q->msix_index >= ioc->iopoll_q_start_index) {
			pr_info("%s enabled: index: %d\n",
			       reply_q->name, reply_q->msix_index);
			continue;
		}
		pr_info("%s %s: IRQ %d\n",
		       reply_q->name,
		       ((ioc->msix_enable) ? "PCI-MSI-X enabled" :
				"IO-APIC enabled"), pci_irq_vector(ioc->pdev,
							   reply_q->msix_index));
	}
	pr_info("%s iomem(%pap), mapped(0x%p), size(%d)\n",
	       ioc->name, &chip_phys, ioc->chip, memap_sz);
	pr_info("%s ioport(0x%016llx), size(%d)\n",
	       ioc->name, (unsigned long long)pio_chip, pio_sz);

	pci_save_state(pdev);
	return 0;
out_fail:
	leapioraid_base_unmap_resources(ioc);
	return r;
}

void *leapioraid_base_get_msg_frame(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid)
{
	return (void *)(ioc->request + (smid * ioc->request_sz));
}

void *leapioraid_base_get_sense_buffer(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid)
{
	return (void *)(ioc->sense + ((smid - 1) * SCSI_SENSE_BUFFERSIZE));
}

__le32
leapioraid_base_get_sense_buffer_dma(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid)
{
	return cpu_to_le32(ioc->sense_dma + ((smid - 1) *
					     SCSI_SENSE_BUFFERSIZE));
}

__le64
leapioraid_base_get_sense_buffer_dma_64(struct LEAPIORAID_ADAPTER *ioc,
					u16 smid)
{
	return cpu_to_le64(ioc->sense_dma + ((smid - 1) *
					     SCSI_SENSE_BUFFERSIZE));
}

void *leapioraid_base_get_reply_virt_addr(struct LEAPIORAID_ADAPTER *ioc,
					  u32 phys_addr)
{
	if (!phys_addr)
		return NULL;
	return ioc->reply + (phys_addr - (u32) ioc->reply_dma);
}

static inline u8
leapioraid_base_get_msix_index(
	struct LEAPIORAID_ADAPTER *ioc, struct scsi_cmnd *scmd)
{
	if (ioc->msix_load_balance)
		return ioc->reply_queue_count ?
		    leapioraid_base_mod64(atomic64_add_return(1, &ioc->total_io_cnt),
			       ioc->reply_queue_count) : 0;
	if (scmd && ioc->shost->nr_hw_queues > 1) {
		u32 tag = blk_mq_unique_tag(scsi_cmd_to_rq(scmd));

		return blk_mq_unique_tag_to_hwq(tag) + ioc->high_iops_queues;
	}
	return ioc->cpu_msix_table[raw_smp_processor_id()];
}

inline unsigned long
leapioraid_base_sdev_nr_inflight_request(struct LEAPIORAID_ADAPTER *ioc,
			       struct scsi_cmnd *scmd)
{
	return scsi_device_busy(scmd->device);
}

static inline u8
leapioraid_base_get_high_iops_msix_index(struct LEAPIORAID_ADAPTER *ioc,
			       struct scsi_cmnd *scmd)
{
	if (leapioraid_base_sdev_nr_inflight_request(ioc, scmd) >
	    LEAPIORAID_DEVICE_HIGH_IOPS_DEPTH)
		return
		    leapioraid_base_mod64((atomic64_add_return
				(1,
				 &ioc->high_iops_outstanding) /
				LEAPIORAID_HIGH_IOPS_BATCH_COUNT),
			       LEAPIORAID_HIGH_IOPS_REPLY_QUEUES);
	return leapioraid_base_get_msix_index(ioc, scmd);
}

u16
leapioraid_base_get_smid(struct LEAPIORAID_ADAPTER *ioc, u8 cb_idx)
{
	unsigned long flags;
	struct leapioraid_request_tracker *request;
	u16 smid;

	spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
	if (list_empty(&ioc->internal_free_list)) {
		spin_unlock_irqrestore(&ioc->scsi_lookup_lock, flags);
		pr_err("%s %s: smid not available\n",
		       ioc->name, __func__);
		return 0;
	}
	request = list_entry(ioc->internal_free_list.next,
			     struct leapioraid_request_tracker, tracker_list);
	request->cb_idx = cb_idx;
	smid = request->smid;
	list_del(&request->tracker_list);
	spin_unlock_irqrestore(&ioc->scsi_lookup_lock, flags);
	return smid;
}

u16
leapioraid_base_get_smid_scsiio(struct LEAPIORAID_ADAPTER *ioc, u8 cb_idx,
				struct scsi_cmnd *scmd)
{
	struct leapioraid_scsiio_tracker *request;
	u16 smid;
	u32 tag = scsi_cmd_to_rq(scmd)->tag;
	u32 unique_tag;

	unique_tag = blk_mq_unique_tag(scsi_cmd_to_rq(scmd));
	tag = blk_mq_unique_tag_to_tag(unique_tag);
	ioc->io_queue_num[tag] = blk_mq_unique_tag_to_hwq(unique_tag);
	request = leapioraid_base_scsi_cmd_priv(scmd);
	smid = tag + 1;
	request->cb_idx = cb_idx;
	request->smid = smid;
	request->scmd = scmd;
	return smid;
}

u16
leapioraid_base_get_smid_hpr(struct LEAPIORAID_ADAPTER *ioc, u8 cb_idx)
{
	unsigned long flags;
	struct leapioraid_request_tracker *request;
	u16 smid;

	spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
	if (list_empty(&ioc->hpr_free_list)) {
		spin_unlock_irqrestore(&ioc->scsi_lookup_lock, flags);
		return 0;
	}
	request = list_entry(ioc->hpr_free_list.next,
			     struct leapioraid_request_tracker, tracker_list);
	request->cb_idx = cb_idx;
	smid = request->smid;
	list_del(&request->tracker_list);
	spin_unlock_irqrestore(&ioc->scsi_lookup_lock, flags);
	return smid;
}

static void
leapioraid_base_recovery_check(struct LEAPIORAID_ADAPTER *ioc)
{
	if (ioc->shost_recovery && ioc->pending_io_count) {
		if (ioc->pending_io_count == 1)
			wake_up(&ioc->reset_wq);
		ioc->pending_io_count--;
	}
}

void
leapioraid_base_clear_st(struct LEAPIORAID_ADAPTER *ioc,
			      struct leapioraid_scsiio_tracker *st)
{
	if (!st)
		return;
	if (WARN_ON(st->smid == 0))
		return;
	st->cb_idx = 0xFF;
	st->direct_io = 0;
	st->scmd = NULL;
	atomic_set(&ioc->chain_lookup[st->smid - 1].chain_offset, 0);
}

void
leapioraid_base_free_smid(struct LEAPIORAID_ADAPTER *ioc, u16 smid)
{
	unsigned long flags;
	int i;
	struct leapioraid_scsiio_tracker *st;
	void *request;

	if (smid < ioc->hi_priority_smid) {
		st = leapioraid_get_st_from_smid(ioc, smid);
		if (!st) {
			leapioraid_base_recovery_check(ioc);
			return;
		}
		request = leapioraid_base_get_msg_frame(ioc, smid);
		memset(request, 0, ioc->request_sz);
		leapioraid_base_clear_st(ioc, st);
		leapioraid_base_recovery_check(ioc);
		ioc->io_queue_num[smid - 1] = 0xFFFF;
		return;
	}
	spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
	if (smid < ioc->internal_smid) {
		i = smid - ioc->hi_priority_smid;
		ioc->hpr_lookup[i].cb_idx = 0xFF;
		list_add(&ioc->hpr_lookup[i].tracker_list, &ioc->hpr_free_list);
	} else if (smid <= ioc->hba_queue_depth) {
		i = smid - ioc->internal_smid;
		ioc->internal_lookup[i].cb_idx = 0xFF;
		list_add(&ioc->internal_lookup[i].tracker_list,
			 &ioc->internal_free_list);
	}
	spin_unlock_irqrestore(&ioc->scsi_lookup_lock, flags);
}

#if defined(writeq) && defined(CONFIG_64BIT)
static inline void
leapioraid_base_writeq(
	__u64 b, void __iomem *addr, spinlock_t *writeq_lock)
{
	writeq(b, addr);
}
#else
static inline void
leapioraid_base_writeq(
	__u64 b, void __iomem *addr, spinlock_t *writeq_lock)
{
	unsigned long flags;
	__u64 data_out = b;

	spin_lock_irqsave(writeq_lock, flags);
	writel((u32) (data_out), addr);
	writel((u32) (data_out >> 32), (addr + 4));
	spin_unlock_irqrestore(writeq_lock, flags);
}
#endif

static u8
leapioraid_base_set_and_get_msix_index(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid)
{
	struct leapioraid_scsiio_tracker *st;

	st = (smid <
	      ioc->hi_priority_smid) ? (leapioraid_get_st_from_smid(ioc,
								    smid))
	    : (NULL);
	if (st == NULL)
		return leapioraid_base_get_msix_index(ioc, NULL);
	st->msix_io = ioc->get_msix_index_for_smlio(ioc, st->scmd);
	return st->msix_io;
}

static void
leapioraid_base_put_smid_scsi_io(struct LEAPIORAID_ADAPTER *ioc, u16 smid,
				 u16 handle)
{
	union LeapioraidReqDescUnion_t descriptor;
	u64 *request = (u64 *) &descriptor;

	descriptor.SCSIIO.RequestFlags = LEAPIORAID_REQ_DESCRIPT_FLAGS_SCSI_IO;
	descriptor.SCSIIO.MSIxIndex
		= leapioraid_base_set_and_get_msix_index(ioc, smid);
	descriptor.SCSIIO.SMID = cpu_to_le16(smid);
	descriptor.SCSIIO.DevHandle = cpu_to_le16(handle);
	descriptor.SCSIIO.LMID = 0;
	leapioraid_base_writeq(*request, &ioc->chip->RequestDescriptorPostLow,
		     &ioc->scsi_lookup_lock);
}

static void
leapioraid_base_put_smid_fast_path(struct LEAPIORAID_ADAPTER *ioc, u16 smid,
				   u16 handle)
{
	union LeapioraidReqDescUnion_t descriptor;
	u64 *request = (u64 *) &descriptor;

	descriptor.SCSIIO.RequestFlags =
	    LEAPIORAID_REQ_DESCRIPT_FLAGS_FAST_PATH_SCSI_IO;
	descriptor.SCSIIO.MSIxIndex
		= leapioraid_base_set_and_get_msix_index(ioc, smid);
	descriptor.SCSIIO.SMID = cpu_to_le16(smid);
	descriptor.SCSIIO.DevHandle = cpu_to_le16(handle);
	descriptor.SCSIIO.LMID = 0;
	leapioraid_base_writeq(*request, &ioc->chip->RequestDescriptorPostLow,
		     &ioc->scsi_lookup_lock);
}

static void
leapioraid_base_put_smid_hi_priority(struct LEAPIORAID_ADAPTER *ioc, u16 smid,
				     u16 msix_task)
{
	union LeapioraidReqDescUnion_t descriptor;
	u64 *request;

	request = (u64 *) &descriptor;
	descriptor.HighPriority.RequestFlags =
	    LEAPIORAID_REQ_DESCRIPT_FLAGS_HIGH_PRIORITY;
	descriptor.HighPriority.MSIxIndex = msix_task;
	descriptor.HighPriority.SMID = cpu_to_le16(smid);
	descriptor.HighPriority.LMID = 0;
	descriptor.HighPriority.Reserved1 = 0;
	leapioraid_base_writeq(*request, &ioc->chip->RequestDescriptorPostLow,
		     &ioc->scsi_lookup_lock);
}

static void
leapioraid_base_put_smid_default(struct LEAPIORAID_ADAPTER *ioc, u16 smid)
{
	union LeapioraidReqDescUnion_t descriptor;
	u64 *request;

	request = (u64 *) &descriptor;
	descriptor.Default.RequestFlags =
	    LEAPIORAID_REQ_DESCRIPT_FLAGS_DEFAULT_TYPE;
	descriptor.Default.MSIxIndex
		= leapioraid_base_set_and_get_msix_index(ioc, smid);
	descriptor.Default.SMID = cpu_to_le16(smid);
	descriptor.Default.LMID = 0;
	descriptor.Default.DescriptorTypeDependent = 0;
	leapioraid_base_writeq(*request, &ioc->chip->RequestDescriptorPostLow,
		     &ioc->scsi_lookup_lock);
}

static void
leapioraid_base_put_smid_scsi_io_atomic(struct LEAPIORAID_ADAPTER *ioc,
					u16 smid, u16 handle)
{
	struct LeapioraidAtomicReqDesc_t descriptor;
	u32 *request = (u32 *) &descriptor;

	descriptor.RequestFlags = LEAPIORAID_REQ_DESCRIPT_FLAGS_SCSI_IO;
	descriptor.MSIxIndex = leapioraid_base_set_and_get_msix_index(ioc, smid);
	descriptor.SMID = cpu_to_le16(smid);
	writel(cpu_to_le32(*request), &ioc->chip->AtomicRequestDescriptorPost);
}

static void
leapioraid_base_put_smid_fast_path_atomic(struct LEAPIORAID_ADAPTER *ioc,
					  u16 smid, u16 handle)
{
	struct LeapioraidAtomicReqDesc_t descriptor;
	u32 *request = (u32 *) &descriptor;

	descriptor.RequestFlags = LEAPIORAID_REQ_DESCRIPT_FLAGS_FAST_PATH_SCSI_IO;
	descriptor.MSIxIndex = leapioraid_base_set_and_get_msix_index(ioc, smid);
	descriptor.SMID = cpu_to_le16(smid);
	writel(cpu_to_le32(*request), &ioc->chip->AtomicRequestDescriptorPost);
}

static void
leapioraid_base_put_smid_hi_priority_atomic(struct LEAPIORAID_ADAPTER *ioc,
					    u16 smid, u16 msix_task)
{
	struct LeapioraidAtomicReqDesc_t descriptor;
	u32 *request = (u32 *) &descriptor;

	descriptor.RequestFlags = LEAPIORAID_REQ_DESCRIPT_FLAGS_HIGH_PRIORITY;
	descriptor.MSIxIndex = msix_task;
	descriptor.SMID = cpu_to_le16(smid);
	writel(cpu_to_le32(*request), &ioc->chip->AtomicRequestDescriptorPost);
}

static void
leapioraid_base_put_smid_default_atomic(struct LEAPIORAID_ADAPTER *ioc,
					u16 smid)
{
	struct LeapioraidAtomicReqDesc_t descriptor;
	u32 *request = (u32 *)(&descriptor);

	descriptor.RequestFlags = LEAPIORAID_REQ_DESCRIPT_FLAGS_DEFAULT_TYPE;
	descriptor.MSIxIndex = leapioraid_base_set_and_get_msix_index(ioc, smid);
	descriptor.SMID = cpu_to_le16(smid);
	writel(cpu_to_le32(*request), &ioc->chip->AtomicRequestDescriptorPost);
}

static int
leapioraid_base_display_fwpkg_version(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidFWImgHeader_t *fw_img_hdr;
	struct LeapioraidComptImgHeader_t *cmp_img_hdr;
	struct LeapioraidFWUploadReq_t *mpi_request;
	struct LeapioraidFWUploadRep_t mpi_reply;
	int r = 0, issue_diag_reset = 0;
	u32 package_version = 0;
	void *fwpkg_data = NULL;
	dma_addr_t fwpkg_data_dma;
	u16 smid, ioc_status;
	size_t data_length;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	if (ioc->base_cmds.status & LEAPIORAID_CMD_PENDING) {
		pr_err("%s %s: internal command already in use\n", ioc->name,
		       __func__);
		return -EAGAIN;
	}
	data_length = sizeof(struct LeapioraidFWImgHeader_t);
	fwpkg_data = dma_alloc_coherent(&ioc->pdev->dev, data_length,
					&fwpkg_data_dma, GFP_ATOMIC);
	if (!fwpkg_data)
		return -ENOMEM;

	smid = leapioraid_base_get_smid(ioc, ioc->base_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		r = -EAGAIN;
		goto out;
	}
	ioc->base_cmds.status = LEAPIORAID_CMD_PENDING;
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->base_cmds.smid = smid;
	memset(mpi_request, 0, sizeof(struct LeapioraidFWUploadReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_FW_UPLOAD;
	mpi_request->ImageType = 0x01;
	mpi_request->ImageSize = data_length;
	ioc->build_sg(ioc, &mpi_request->SGL, 0, 0, fwpkg_data_dma,
		      data_length);
	init_completion(&ioc->base_cmds.done);
	ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->base_cmds.done, 15 * HZ);
	dinitprintk(ioc, pr_info("%s %s: complete\n",
				ioc->name, __func__));
	if (!(ioc->base_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		pr_err("%s %s: timeout\n",
		       ioc->name, __func__);
		leapioraid_debug_dump_mf(mpi_request,
			       sizeof(struct LeapioraidFWUploadReq_t) / 4);
		issue_diag_reset = 1;
	} else {
		memset(&mpi_reply, 0, sizeof(struct LeapioraidFWUploadRep_t));
		if (ioc->base_cmds.status & LEAPIORAID_CMD_REPLY_VALID) {
			memcpy(&mpi_reply, ioc->base_cmds.reply,
			       sizeof(struct LeapioraidFWUploadRep_t));
			ioc_status = le16_to_cpu(mpi_reply.IOCStatus) &
			    LEAPIORAID_IOCSTATUS_MASK;
			if (ioc_status == LEAPIORAID_IOCSTATUS_SUCCESS) {
				fw_img_hdr =
				    (struct LeapioraidFWImgHeader_t *) fwpkg_data;
				if (le32_to_cpu(fw_img_hdr->Signature) ==
				    0xEB000042) {
					cmp_img_hdr =
					    (struct LeapioraidComptImgHeader_t
					     *) (fwpkg_data);
					package_version =
					    le32_to_cpu(cmp_img_hdr->ApplicationSpecific);
				} else
					package_version =
					    le32_to_cpu(fw_img_hdr->PackageVersion.Word);
				if (package_version)
					pr_err(
					       "%s FW Package Version(%02d.%02d.%02d.%02d)\n",
					       ioc->name,
					       ((package_version) & 0xFF000000)
					       >> 24,
					       ((package_version) & 0x00FF0000)
					       >> 16,
					       ((package_version) & 0x0000FF00)
					       >> 8,
					       (package_version) & 0x000000FF);
			} else {
				leapioraid_debug_dump_mf(&mpi_reply,
					       sizeof(struct LeapioraidFWUploadRep_t) /
					       4);
			}
		}
	}
	ioc->base_cmds.status = LEAPIORAID_CMD_NOT_USED;
out:
	if (fwpkg_data)
		dma_free_coherent(&ioc->pdev->dev, data_length, fwpkg_data,
				  fwpkg_data_dma);
	if (issue_diag_reset) {
		if (ioc->drv_internal_flags & LEAPIORAID_DRV_INERNAL_FIRST_PE_ISSUED)
			return -EFAULT;
		if (leapioraid_base_check_for_fault_and_issue_reset(ioc))
			return -EFAULT;
		r = -EAGAIN;
	}
	return r;
}

static void
leapioraid_base_display_ioc_capabilities(struct LEAPIORAID_ADAPTER *ioc)
{
	int i = 0;
	char desc[17] = { 0 };
	u8 revision;
	u32 iounit_pg1_flags;

	pci_read_config_byte(ioc->pdev, PCI_CLASS_REVISION, &revision);
	strscpy(desc, ioc->manu_pg0.ChipName, sizeof(desc));
	pr_info("%s %s: FWVersion(%02d.%02d.%02d.%02d), ChipRevision(0x%02x)\n",
	       ioc->name, desc,
	       (ioc->facts.FWVersion.Word & 0xFF000000) >> 24,
	       (ioc->facts.FWVersion.Word & 0x00FF0000) >> 16,
	       (ioc->facts.FWVersion.Word & 0x0000FF00) >> 8,
	       ioc->facts.FWVersion.Word & 0x000000FF, revision);
	pr_info("%s Protocol=(", ioc->name);
	if (ioc->facts.ProtocolFlags & LEAPIORAID_IOCFACTS_PROTOCOL_SCSI_INITIATOR) {
		pr_info("Initiator");
		i++;
	}
	if (ioc->facts.ProtocolFlags & LEAPIORAID_IOCFACTS_PROTOCOL_SCSI_TARGET) {
		pr_info("%sTarget", i ? "," : "");
		i++;
	}
	i = 0;
	pr_info("), ");
	pr_info("Capabilities=(");
	if ((!ioc->warpdrive_msg) && (ioc->facts.IOCCapabilities &
				      LEAPIORAID_IOCFACTS_CAPABILITY_INTEGRATED_RAID)) {
		pr_info("Raid");
		i++;
	}
	if (ioc->facts.IOCCapabilities & LEAPIORAID_IOCFACTS_CAPABILITY_TLR) {
		pr_info("%sTLR", i ? "," : "");
		i++;
	}
	if (ioc->facts.IOCCapabilities & LEAPIORAID_IOCFACTS_CAPABILITY_MULTICAST) {
		pr_info("%sMulticast", i ? "," : "");
		i++;
	}
	if (ioc->facts.IOCCapabilities &
	    LEAPIORAID_IOCFACTS_CAPABILITY_BIDIRECTIONAL_TARGET) {
		pr_info("%sBIDI Target", i ? "," : "");
		i++;
	}
	if (ioc->facts.IOCCapabilities & LEAPIORAID_IOCFACTS_CAPABILITY_EEDP) {
		pr_info("%sEEDP", i ? "," : "");
		i++;
	}
	if (ioc->facts.IOCCapabilities &
	    LEAPIORAID_IOCFACTS_CAPABILITY_TASK_SET_FULL_HANDLING) {
		pr_info("%sTask Set Full", i ? "," : "");
		i++;
	}
	iounit_pg1_flags = le32_to_cpu(ioc->iounit_pg1.Flags);
	if (!(iounit_pg1_flags & LEAPIORAID_IOUNITPAGE1_NATIVE_COMMAND_Q_DISABLE)) {
		pr_info("%sNCQ", i ? "," : "");
		i++;
	}
	pr_info(")\n");
}

static int
leapioraid_base_update_ioc_page1_inlinewith_perf_mode(
	struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidIOCP1_t ioc_pg1;
	struct LeapioraidCfgRep_t mpi_reply;
	int rc;

	rc = leapioraid_config_get_ioc_pg1(ioc, &mpi_reply, &ioc->ioc_pg1_copy);
	if (rc)
		return rc;
	memcpy(&ioc_pg1, &ioc->ioc_pg1_copy, sizeof(struct LeapioraidIOCP1_t));
	switch (perf_mode) {
	case LEAPIORAID_PERF_MODE_DEFAULT:
	case LEAPIORAID_PERF_MODE_BALANCED:
		if (ioc->high_iops_queues) {
			pr_err(
				"%s Enable int coalescing only for first %d reply queues\n",
					ioc->name, LEAPIORAID_HIGH_IOPS_REPLY_QUEUES);
			ioc_pg1.ProductSpecific = cpu_to_le32(0x80000000 |
							      ((1 <<
								LEAPIORAID_HIGH_IOPS_REPLY_QUEUES
								/ 8) - 1));
			rc = leapioraid_config_set_ioc_pg1(ioc, &mpi_reply,
							   &ioc_pg1);
			if (rc)
				return rc;
			pr_err("%s performance mode: balanced\n", ioc->name);
			return 0;
		}
		fallthrough;
	case LEAPIORAID_PERF_MODE_LATENCY:
		ioc_pg1.CoalescingTimeout = cpu_to_le32(0xa);
		ioc_pg1.Flags |= cpu_to_le32(0x00000001);
		ioc_pg1.ProductSpecific = 0;
		rc = leapioraid_config_set_ioc_pg1(ioc, &mpi_reply, &ioc_pg1);
		if (rc)
			return rc;
		pr_err("%s performance mode: latency\n", ioc->name);
		break;
	case LEAPIORAID_PERF_MODE_IOPS:
		pr_err(
		       "%s performance mode: iops with coalescing timeout: 0x%x\n",
		       ioc->name, le32_to_cpu(ioc_pg1.CoalescingTimeout));
		ioc_pg1.Flags |= cpu_to_le32(0x00000001);
		ioc_pg1.ProductSpecific = 0;
		rc = leapioraid_config_set_ioc_pg1(ioc, &mpi_reply, &ioc_pg1);
		if (rc)
			return rc;
		break;
	}
	return 0;
}

static int
leapioraid_base_assign_fw_reported_qd(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidSasIOUnitP1_t *sas_iounit_pg1 = NULL;
	int sz;
	int rc = 0;

	ioc->max_wideport_qd = LEAPIORAID_SAS_QUEUE_DEPTH;
	ioc->max_narrowport_qd = LEAPIORAID_SAS_QUEUE_DEPTH;
	ioc->max_sata_qd = LEAPIORAID_SATA_QUEUE_DEPTH;

	sz = offsetof(struct LeapioraidSasIOUnitP1_t, PhyData);
	sas_iounit_pg1 = kzalloc(sz, GFP_KERNEL);
	if (!sas_iounit_pg1) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		return rc;
	}
	rc = leapioraid_config_get_sas_iounit_pg1(ioc, &mpi_reply,
						  sas_iounit_pg1, sz);
	if (rc) {
		pr_err("%s failure at %s:%d/%s()!\n",
		       ioc->name, __FILE__, __LINE__, __func__);
		goto out;
	}
	ioc->max_wideport_qd =
	    (le16_to_cpu(sas_iounit_pg1->SASWideMaxQueueDepth)) ?
	    le16_to_cpu(sas_iounit_pg1->SASWideMaxQueueDepth) :
	    LEAPIORAID_SAS_QUEUE_DEPTH;
	ioc->max_narrowport_qd =
	    (le16_to_cpu(sas_iounit_pg1->SASNarrowMaxQueueDepth)) ?
	    le16_to_cpu(sas_iounit_pg1->SASNarrowMaxQueueDepth) :
	    LEAPIORAID_SAS_QUEUE_DEPTH;
	ioc->max_sata_qd = (sas_iounit_pg1->SATAMaxQDepth) ?
	    sas_iounit_pg1->SATAMaxQDepth : LEAPIORAID_SATA_QUEUE_DEPTH;
out:
	dinitprintk(ioc, pr_err(
			"%s MaxWidePortQD: 0x%x MaxNarrowPortQD: 0x%x MaxSataQD: 0x%x\n",
			ioc->name, ioc->max_wideport_qd,
			ioc->max_narrowport_qd, ioc->max_sata_qd));
	kfree(sas_iounit_pg1);
	return rc;
}

static int
leapioraid_base_static_config_pages(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidCfgRep_t mpi_reply;
	u32 iounit_pg1_flags;
	int rc;

	rc = leapioraid_config_get_manufacturing_pg0(ioc, &mpi_reply,
						     &ioc->manu_pg0);
	if (rc)
		return rc;
	if (ioc->ir_firmware) {
		rc = leapioraid_config_get_manufacturing_pg10(ioc, &mpi_reply,
							      &ioc->manu_pg10);
		if (rc)
			return rc;
	}
	rc = leapioraid_config_get_manufacturing_pg11(ioc, &mpi_reply,
						      &ioc->manu_pg11);
	if (rc)
		return rc;

	ioc->time_sync_interval =
	    ioc->manu_pg11.TimeSyncInterval & 0x7F;
	if (ioc->time_sync_interval) {
		if (ioc->manu_pg11.TimeSyncInterval & 0x80)
			ioc->time_sync_interval =
			    ioc->time_sync_interval * 3600;
		else
			ioc->time_sync_interval =
			    ioc->time_sync_interval * 60;
		dinitprintk(ioc, pr_info(
			"%s Driver-FW TimeSync interval is %d seconds.\n\t\t"
				"ManuPg11 TimeSync Unit is in %s's",
					ioc->name,
					ioc->time_sync_interval,
					((ioc->manu_pg11.TimeSyncInterval & 0x80)
						? "Hour" : "Minute")));
	}
	rc = leapioraid_base_assign_fw_reported_qd(ioc);
	if (rc)
		return rc;
	rc = leapioraid_config_get_bios_pg2(ioc, &mpi_reply, &ioc->bios_pg2);
	if (rc)
		return rc;
	rc = leapioraid_config_get_bios_pg3(ioc, &mpi_reply, &ioc->bios_pg3);
	if (rc)
		return rc;
	rc = leapioraid_config_get_ioc_pg8(ioc, &mpi_reply, &ioc->ioc_pg8);
	if (rc)
		return rc;
	rc = leapioraid_config_get_iounit_pg0(ioc, &mpi_reply,
					      &ioc->iounit_pg0);
	if (rc)
		return rc;
	rc = leapioraid_config_get_iounit_pg1(ioc, &mpi_reply,
					      &ioc->iounit_pg1);
	if (rc)
		return rc;
	rc = leapioraid_config_get_iounit_pg8(ioc, &mpi_reply,
					      &ioc->iounit_pg8);
	if (rc)
		return rc;
	leapioraid_base_display_ioc_capabilities(ioc);
	iounit_pg1_flags = le32_to_cpu(ioc->iounit_pg1.Flags);
	if ((ioc->facts.IOCCapabilities &
	     LEAPIORAID_IOCFACTS_CAPABILITY_TASK_SET_FULL_HANDLING))
		iounit_pg1_flags &=
		    ~LEAPIORAID_IOUNITPAGE1_DISABLE_TASK_SET_FULL_HANDLING;
	else
		iounit_pg1_flags |=
		    LEAPIORAID_IOUNITPAGE1_DISABLE_TASK_SET_FULL_HANDLING;
	ioc->iounit_pg1.Flags = cpu_to_le32(iounit_pg1_flags);
	rc = leapioraid_config_set_iounit_pg1(ioc, &mpi_reply,
					      &ioc->iounit_pg1);
	if (rc)
		return rc;
	if (ioc->iounit_pg8.NumSensors)
		ioc->temp_sensors_count = ioc->iounit_pg8.NumSensors;

	rc = leapioraid_base_update_ioc_page1_inlinewith_perf_mode(ioc);
	if (rc)
		return rc;

	return 0;
}

void
leapioraid_free_enclosure_list(struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_enclosure_node *enclosure_dev, *enclosure_dev_next;

	list_for_each_entry_safe(enclosure_dev,
				 enclosure_dev_next, &ioc->enclosure_list,
				 list) {
		list_del(&enclosure_dev->list);
		kfree(enclosure_dev);
	}
}

static void
leapioraid_base_release_memory_pools(struct LEAPIORAID_ADAPTER *ioc)
{
	int i, j;
	int dma_alloc_count = 0;
	struct leapioraid_chain_tracker *ct;
	int count = ioc->rdpq_array_enable ? ioc->reply_queue_count : 1;

	dexitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	if (ioc->request) {
		dma_free_coherent(&ioc->pdev->dev, ioc->request_dma_sz,
				  ioc->request, ioc->request_dma);
		dexitprintk(ioc,
			    pr_info("%s request_pool(0x%p): free\n",
					ioc->name, ioc->request));
		ioc->request = NULL;
	}
	if (ioc->sense) {
		dma_pool_free(ioc->sense_dma_pool, ioc->sense, ioc->sense_dma);
		dma_pool_destroy(ioc->sense_dma_pool);
		dexitprintk(ioc, pr_info("%s sense_pool(0x%p): free\n",
			ioc->name, ioc->sense));
		ioc->sense = NULL;
	}
	if (ioc->reply) {
		dma_pool_free(ioc->reply_dma_pool, ioc->reply, ioc->reply_dma);
		dma_pool_destroy(ioc->reply_dma_pool);
		dexitprintk(ioc, pr_info("%s reply_pool(0x%p): free\n",
			ioc->name, ioc->reply));
		ioc->reply = NULL;
	}
	if (ioc->reply_free) {
		dma_pool_free(ioc->reply_free_dma_pool, ioc->reply_free,
			      ioc->reply_free_dma);
		dma_pool_destroy(ioc->reply_free_dma_pool);
		dexitprintk(ioc, pr_info("%s reply_free_pool(0x%p): free\n",
			ioc->name, ioc->reply_free));
		ioc->reply_free = NULL;
	}
	if (ioc->reply_post) {
		dma_alloc_count = DIV_ROUND_UP(count,
					       LEAPIORAID_RDPQ_MAX_INDEX_IN_ONE_CHUNK);
		for (i = 0; i < count; i++) {
			if (i % LEAPIORAID_RDPQ_MAX_INDEX_IN_ONE_CHUNK == 0
			    && dma_alloc_count) {
				if (ioc->reply_post[i].reply_post_free) {
					dma_pool_free(ioc->reply_post_free_dma_pool,
						      ioc->reply_post[i].reply_post_free,
						      ioc->reply_post[i].reply_post_free_dma);
					pr_err(
					       "%s reply_post_free_pool(0x%p): free\n",
					       ioc->name,
					       ioc->reply_post[i].reply_post_free);
					ioc->reply_post[i].reply_post_free =
					    NULL;
				}
				--dma_alloc_count;
			}
		}
		dma_pool_destroy(ioc->reply_post_free_dma_pool);
		if (ioc->reply_post_free_array && ioc->rdpq_array_enable) {
			dma_pool_free(ioc->reply_post_free_array_dma_pool,
				      ioc->reply_post_free_array,
				      ioc->reply_post_free_array_dma);
			ioc->reply_post_free_array = NULL;
		}
		dma_pool_destroy(ioc->reply_post_free_array_dma_pool);
		kfree(ioc->reply_post);
	}
	if (ioc->config_page) {
		dexitprintk(ioc, pr_err(
					"%s config_page(0x%p): free\n", ioc->name,
					ioc->config_page));
		dma_free_coherent(&ioc->pdev->dev, ioc->config_page_sz,
				  ioc->config_page, ioc->config_page_dma);
	}
	kfree(ioc->hpr_lookup);
	kfree(ioc->internal_lookup);
	if (ioc->chain_lookup) {
		for (i = 0; i < ioc->scsiio_depth; i++) {
			for (j = ioc->chains_per_prp_buffer;
			     j < ioc->chains_needed_per_io; j++) {
				ct = &ioc->chain_lookup[i].chains_per_smid[j];
				if (ct && ct->chain_buffer)
					dma_pool_free(ioc->chain_dma_pool,
						      ct->chain_buffer,
						      ct->chain_buffer_dma);
			}
			kfree(ioc->chain_lookup[i].chains_per_smid);
		}
		dma_pool_destroy(ioc->chain_dma_pool);
		kfree(ioc->chain_lookup);
		ioc->chain_lookup = NULL;
	}
	kfree(ioc->io_queue_num);
	ioc->io_queue_num = NULL;
}

static int
leapioraid_check_same_4gb_region(dma_addr_t start_address, u32 pool_sz)
{
	dma_addr_t end_address;

	end_address = start_address + pool_sz - 1;
	if (upper_32_bits(start_address) == upper_32_bits(end_address))
		return 1;
	else
		return 0;
}

static inline int
leapioraid_base_reduce_hba_queue_depth(struct LEAPIORAID_ADAPTER *ioc)
{
	int reduce_sz = 64;

	if ((ioc->hba_queue_depth - reduce_sz) >
	    (ioc->internal_depth + LEAPIORAID_INTERNAL_SCSIIO_CMDS_COUNT)) {
		ioc->hba_queue_depth -= reduce_sz;
		return 0;
	} else
		return -ENOMEM;
}

static int
leapioraid_base_allocate_reply_post_free_array(struct LEAPIORAID_ADAPTER *ioc,
					       int reply_post_free_array_sz)
{
	ioc->reply_post_free_array_dma_pool =
	    dma_pool_create("reply_post_free_array pool",
			    &ioc->pdev->dev, reply_post_free_array_sz, 16, 0);
	if (!ioc->reply_post_free_array_dma_pool) {
		dinitprintk(ioc,
			    pr_err
			    ("reply_post_free_array pool: dma_pool_create failed\n"));
		return -ENOMEM;
	}
	ioc->reply_post_free_array =
	    dma_pool_alloc(ioc->reply_post_free_array_dma_pool,
			   GFP_KERNEL, &ioc->reply_post_free_array_dma);
	if (!ioc->reply_post_free_array) {
		dinitprintk(ioc,
			    pr_err
			    ("reply_post_free_array pool: dma_pool_alloc failed\n"));
		return -EAGAIN;
	}
	if (!leapioraid_check_same_4gb_region(ioc->reply_post_free_array_dma,
					      reply_post_free_array_sz)) {
		dinitprintk(ioc, pr_err(
			"Bad Reply Free Pool! Reply Free (0x%p)\n\t\t"
			"Reply Free dma = (0x%llx)\n",
				ioc->reply_free,
				(unsigned long long)ioc->reply_free_dma));
		ioc->use_32bit_dma = 1;
		return -EAGAIN;
	}
	return 0;
}

static int
base_alloc_rdpq_dma_pool(struct LEAPIORAID_ADAPTER *ioc, int sz)
{
	int i = 0;
	u32 dma_alloc_count = 0;
	int reply_post_free_sz = ioc->reply_post_queue_depth *
	    sizeof(struct LeapioraidDefaultRepDesc_t);
	int count = ioc->rdpq_array_enable ? ioc->reply_queue_count : 1;

	ioc->reply_post =
	    kcalloc(count, sizeof(struct leapioraid_reply_post_struct), GFP_KERNEL);
	if (!ioc->reply_post) {
		pr_err("%s reply_post_free pool: kcalloc failed\n", ioc->name);
		return -ENOMEM;
	}
	dma_alloc_count = DIV_ROUND_UP(
		count, LEAPIORAID_RDPQ_MAX_INDEX_IN_ONE_CHUNK);
	ioc->reply_post_free_dma_pool =
	    dma_pool_create("reply_post_free pool", &ioc->pdev->dev, sz, 16, 0);
	if (!ioc->reply_post_free_dma_pool) {
		pr_err("reply_post_free pool: dma_pool_create failed\n");
		return -ENOMEM;
	}
	for (i = 0; i < count; i++) {
		if ((i % LEAPIORAID_RDPQ_MAX_INDEX_IN_ONE_CHUNK == 0) && dma_alloc_count) {
			ioc->reply_post[i].reply_post_free =
			    dma_pool_zalloc(ioc->reply_post_free_dma_pool,
					    GFP_KERNEL,
					    &ioc->reply_post[i].reply_post_free_dma);
			if (!ioc->reply_post[i].reply_post_free) {
				pr_err("reply_post_free pool: dma_pool_alloc failed\n");
				return -EAGAIN;
			}
			if (!leapioraid_check_same_4gb_region
			    (ioc->reply_post[i].reply_post_free_dma, sz)) {
				dinitprintk(ioc, pr_err(
					"%s bad Replypost free pool(0x%p) dma = (0x%llx)\n",
						ioc->name,
						ioc->reply_post[i].reply_post_free,
						(unsigned long long)
						ioc->reply_post[i].reply_post_free_dma));
				ioc->use_32bit_dma = 1;
				return -EAGAIN;
			}
			dma_alloc_count--;
		} else {
			ioc->reply_post[i].reply_post_free =
			    (union LeapioraidRepDescUnion_t *)
			    ((long)ioc->reply_post[i - 1].reply_post_free
			     + reply_post_free_sz);
			ioc->reply_post[i].reply_post_free_dma = (dma_addr_t)
			    (ioc->reply_post[i - 1].reply_post_free_dma +
			     reply_post_free_sz);
		}
	}
	return 0;
}

static int
leapioraid_base_allocate_chain_dma_pool(struct LEAPIORAID_ADAPTER *ioc, int sz)
{
	int i = 0, j = 0;
	struct leapioraid_chain_tracker *ctr;

	ioc->chain_dma_pool = dma_pool_create("chain pool", &ioc->pdev->dev,
					      ioc->chain_segment_sz, 16, 0);
	if (!ioc->chain_dma_pool) {
		pr_err("%s chain_dma_pool: dma_pool_create failed\n", ioc->name);
		return -ENOMEM;
	}
	for (i = 0; i < ioc->scsiio_depth; i++) {
		for (j = ioc->chains_per_prp_buffer;
		     j < ioc->chains_needed_per_io; j++) {
			ctr = &ioc->chain_lookup[i].chains_per_smid[j];
			ctr->chain_buffer = dma_pool_alloc(ioc->chain_dma_pool,
							   GFP_KERNEL,
							   &ctr->chain_buffer_dma);
			if (!ctr->chain_buffer)
				return -EAGAIN;
			if (!leapioraid_check_same_4gb_region
			    (ctr->chain_buffer_dma, ioc->chain_segment_sz)) {
				pr_err(
					"%s buffers not in same 4G! buff=(0x%p) dma=(0x%llx)\n",
						ioc->name,
						ctr->chain_buffer,
						(unsigned long long)ctr->chain_buffer_dma);
				ioc->use_32bit_dma = 1;
				return -EAGAIN;
			}
		}
	}
	dinitprintk(ioc, pr_info(
		"%s chain_lookup depth(%d), frame_size(%d), pool_size(%d kB)\n",
		ioc->name, ioc->scsiio_depth,
		ioc->chain_segment_sz,
		((ioc->scsiio_depth *
			(ioc->chains_needed_per_io -
			ioc->chains_per_prp_buffer) *
			ioc->chain_segment_sz)) / 1024));
	return 0;
}

static int
leapioraid_base_allocate_sense_dma_pool(struct LEAPIORAID_ADAPTER *ioc, int sz)
{
	ioc->sense_dma_pool =
	    dma_pool_create("sense pool", &ioc->pdev->dev, sz, 4, 0);
	if (!ioc->sense_dma_pool) {
		pr_err("%s sense pool: dma_pool_create failed\n", ioc->name);
		return -ENOMEM;
	}
	ioc->sense = dma_pool_alloc(ioc->sense_dma_pool,
				    GFP_KERNEL, &ioc->sense_dma);
	if (!ioc->sense) {
		pr_err("%s sense pool: dma_pool_alloc failed\n", ioc->name);
		return -EAGAIN;
	}
	if (!leapioraid_check_same_4gb_region(ioc->sense_dma, sz)) {
		dinitprintk(ioc,
			    pr_err("Bad Sense Pool! sense (0x%p) sense_dma = (0x%llx)\n",
				   ioc->sense,
				   (unsigned long long)ioc->sense_dma));
		ioc->use_32bit_dma = 1;
		return -EAGAIN;
	}
	pr_err(
		"%s sense pool(0x%p) - dma(0x%llx): depth(%d),\n\t\t"
		"element_size(%d), pool_size (%d kB)\n",
			ioc->name,
			ioc->sense,
			(unsigned long long)ioc->sense_dma,
			ioc->scsiio_depth,
			SCSI_SENSE_BUFFERSIZE, sz / 1024);
	return 0;
}

static int
leapioraid_base_allocate_reply_free_dma_pool(struct LEAPIORAID_ADAPTER *ioc,
					     int sz)
{
	ioc->reply_free_dma_pool =
	    dma_pool_create("reply_free pool", &ioc->pdev->dev, sz, 16, 0);
	if (!ioc->reply_free_dma_pool) {
		pr_err("%s reply_free pool: dma_pool_create failed\n", ioc->name);
		return -ENOMEM;
	}
	ioc->reply_free = dma_pool_alloc(ioc->reply_free_dma_pool,
					 GFP_KERNEL, &ioc->reply_free_dma);
	if (!ioc->reply_free) {
		pr_err("%s reply_free pool: dma_pool_alloc failed\n", ioc->name);
		return -EAGAIN;
	}
	if (!leapioraid_check_same_4gb_region(ioc->reply_free_dma, sz)) {
		dinitprintk(ioc, pr_err(
			"Bad Reply Free Pool! Reply Free (0x%p)\n\t\t"
			"Reply Free dma = (0x%llx)\n",
				ioc->reply_free,
				(unsigned long long)ioc->reply_free_dma));
		ioc->use_32bit_dma = 1;
		return -EAGAIN;
	}
	memset(ioc->reply_free, 0, sz);
	dinitprintk(ioc, pr_info(
		"%s reply_free pool(0x%p): depth(%d),\n\t\t"
		"element_size(%d), pool_size(%d kB)\n",
			ioc->name,
			ioc->reply_free,
			ioc->reply_free_queue_depth, 4,
			sz / 1024));
	dinitprintk(ioc,
		pr_info("%s reply_free_dma (0x%llx)\n",
			ioc->name, (unsigned long long)ioc->reply_free_dma));
	return 0;
}

static int
leapioraid_base_allocate_reply_pool(struct LEAPIORAID_ADAPTER *ioc, int sz)
{
	ioc->reply_dma_pool = dma_pool_create("reply pool",
					      &ioc->pdev->dev, sz, 4, 0);
	if (!ioc->reply_dma_pool) {
		pr_err("%s reply pool: dma_pool_create failed\n", ioc->name);
		return -ENOMEM;
	}
	ioc->reply = dma_pool_alloc(ioc->reply_dma_pool, GFP_KERNEL,
				    &ioc->reply_dma);
	if (!ioc->reply) {
		pr_err("%s reply pool: dma_pool_alloc failed\n", ioc->name);
		return -EAGAIN;
	}
	if (!leapioraid_check_same_4gb_region(ioc->reply_dma, sz)) {
		dinitprintk(ioc,
			    pr_err("Bad Reply Pool! Reply (0x%p) Reply dma = (0x%llx)\n",
				   ioc->reply,
				   (unsigned long long)ioc->reply_dma));
		ioc->use_32bit_dma = 1;
		return -EAGAIN;
	}
	ioc->reply_dma_min_address = (u32) (ioc->reply_dma);
	ioc->reply_dma_max_address = (u32) (ioc->reply_dma) + sz;
	pr_err(
		"%s reply pool(0x%p) - dma(0x%llx): depth(%d)\n\t\t"
		"frame_size(%d), pool_size(%d kB)\n",
			ioc->name,
			ioc->reply,
			(unsigned long long)ioc->reply_dma,
			ioc->reply_free_queue_depth,
			ioc->reply_sz,
			sz / 1024);
	return 0;
}

static int
leapioraid_base_allocate_memory_pools(struct LEAPIORAID_ADAPTER *ioc)
{
	struct leapioraid_facts *facts;
	u16 max_sge_elements;
	u16 chains_needed_per_io;
	u32 sz, total_sz, reply_post_free_sz, rc = 0;
	u32 retry_sz;
	u32 rdpq_sz = 0, sense_sz = 0, reply_post_free_array_sz = 0;
	u16 max_request_credit;
	unsigned short sg_tablesize;
	u16 sge_size;
	int i = 0;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	retry_sz = 0;
	facts = &ioc->facts;
	sg_tablesize = LEAPIORAID_SG_DEPTH;
	if (reset_devices)
		sg_tablesize = min_t(unsigned short, sg_tablesize,
				     LEAPIORAID_KDUMP_MIN_PHYS_SEGMENTS);
	if (sg_tablesize < LEAPIORAID_MIN_PHYS_SEGMENTS)
		sg_tablesize = LEAPIORAID_MIN_PHYS_SEGMENTS;
	else if (sg_tablesize > LEAPIORAID_MAX_PHYS_SEGMENTS) {
		sg_tablesize = min_t(unsigned short, sg_tablesize,
				     LEAPIORAID_MAX_SG_SEGMENTS);
		pr_warn(
			"%s sg_tablesize(%u) is bigger than kernel defined %s(%u)\n",
			ioc->name,
		    sg_tablesize, LEAPIORAID_MAX_PHYS_SEGMENTS_STRING,
		    LEAPIORAID_MAX_PHYS_SEGMENTS);
	}
	ioc->shost->sg_tablesize = sg_tablesize;
	ioc->internal_depth = min_t(int, (facts->HighPriorityCredit + (5)),
				    (facts->RequestCredit / 4));
	if (ioc->internal_depth < LEAPIORAID_INTERNAL_CMDS_COUNT) {
		if (facts->RequestCredit <= (LEAPIORAID_INTERNAL_CMDS_COUNT +
					     LEAPIORAID_INTERNAL_SCSIIO_CMDS_COUNT)) {
			pr_err(
				"%s RequestCredits not enough, it has %d credits\n",
					ioc->name,
					facts->RequestCredit);
			return -ENOMEM;
		}
		ioc->internal_depth = 10;
	}
	ioc->hi_priority_depth = ioc->internal_depth - (5);
	if (reset_devices)
		max_request_credit = min_t(u16, facts->RequestCredit,
					   (LEAPIORAID_KDUMP_SCSI_IO_DEPTH +
					    ioc->internal_depth));
	else
		max_request_credit = min_t(u16, facts->RequestCredit,
					   LEAPIORAID_MAX_HBA_QUEUE_DEPTH);
retry:
	ioc->hba_queue_depth = max_request_credit + ioc->hi_priority_depth;
	ioc->request_sz = facts->IOCRequestFrameSize * 4;
	ioc->reply_sz = facts->ReplyFrameSize * 4;
	if (facts->IOCMaxChainSegmentSize)
		ioc->chain_segment_sz =
		    facts->IOCMaxChainSegmentSize * LEAPIORAID_MAX_CHAIN_ELEMT_SZ;
	else
		ioc->chain_segment_sz =
		    LEAPIORAID_DEFAULT_NUM_FWCHAIN_ELEMTS * LEAPIORAID_MAX_CHAIN_ELEMT_SZ;
	sge_size = max_t(u16, ioc->sge_size, ioc->sge_size_ieee);
retry_allocation:
	total_sz = 0;
	max_sge_elements =
	    ioc->request_sz -
	    ((sizeof(struct LeapioraidSCSIIOReq_t) -
			sizeof(union LEAPIORAID_IEEE_SGE_IO_UNION)) + 2 * sge_size);
	ioc->max_sges_in_main_message = max_sge_elements / sge_size;
	max_sge_elements = ioc->chain_segment_sz - sge_size;
	ioc->max_sges_in_chain_message = max_sge_elements / sge_size;
	chains_needed_per_io = ((ioc->shost->sg_tablesize -
				 ioc->max_sges_in_main_message) /
				ioc->max_sges_in_chain_message)
	    + 1;
	if (chains_needed_per_io > facts->MaxChainDepth) {
		chains_needed_per_io = facts->MaxChainDepth;
		ioc->shost->sg_tablesize = min_t(u16,
						 ioc->max_sges_in_main_message +
						 (ioc->max_sges_in_chain_message *
						  chains_needed_per_io),
						 ioc->shost->sg_tablesize);
	}
	ioc->chains_needed_per_io = chains_needed_per_io;
	ioc->reply_free_queue_depth = ioc->hba_queue_depth + 64;
	ioc->reply_post_queue_depth = ioc->hba_queue_depth +
	    ioc->reply_free_queue_depth + 1;
	if (ioc->reply_post_queue_depth % 16)
		ioc->reply_post_queue_depth +=
		    16 - (ioc->reply_post_queue_depth % 16);
	if (ioc->reply_post_queue_depth >
	    facts->MaxReplyDescriptorPostQueueDepth) {
		ioc->reply_post_queue_depth =
		    facts->MaxReplyDescriptorPostQueueDepth -
		    (facts->MaxReplyDescriptorPostQueueDepth % 16);
		ioc->hba_queue_depth =
		    ((ioc->reply_post_queue_depth - 64) / 2) - 1;
		ioc->reply_free_queue_depth = ioc->hba_queue_depth + 64;
	}
	pr_info(
		"%s scatter gather: sge_in_main_msg(%d),\n\t\t"
		"sge_per_chain(%d), sge_per_io(%d), chains_per_io(%d)\n",
			ioc->name,
			ioc->max_sges_in_main_message,
			ioc->max_sges_in_chain_message,
			ioc->shost->sg_tablesize,
			ioc->chains_needed_per_io);
	ioc->scsiio_depth = ioc->hba_queue_depth -
	    ioc->hi_priority_depth - ioc->internal_depth;
	ioc->shost->can_queue =
		ioc->scsiio_depth - LEAPIORAID_INTERNAL_SCSIIO_CMDS_COUNT;
	dinitprintk(ioc, pr_info("%s scsi host: can_queue depth (%d)\n", ioc->name,
				ioc->shost->can_queue));
	sz = ((ioc->scsiio_depth + 1) * ioc->request_sz);
	sz += (ioc->hi_priority_depth * ioc->request_sz);
	sz += (ioc->internal_depth * ioc->request_sz);
	ioc->request_dma_sz = sz;
	ioc->request = dma_alloc_coherent(&ioc->pdev->dev, sz,
					  &ioc->request_dma, GFP_KERNEL);
	if (!ioc->request) {
		if (ioc->scsiio_depth < LEAPIORAID_SAS_QUEUE_DEPTH) {
			rc = -ENOMEM;
			goto out;
		}
		retry_sz = 64;
		if ((ioc->hba_queue_depth - retry_sz) >
		    (ioc->internal_depth + LEAPIORAID_INTERNAL_SCSIIO_CMDS_COUNT)) {
			ioc->hba_queue_depth -= retry_sz;
			goto retry_allocation;
		} else {
			rc = -ENOMEM;
			goto out;
		}
	}
	memset(ioc->request, 0, sz);
	if (retry_sz)
		pr_err(
			"%s request pool: dma_alloc_consistent succeed:\n\t\t"
			"hba_depth(%d), chains_per_io(%d), frame_sz(%d), total(%d kb)\n",
				ioc->name,
				ioc->hba_queue_depth,
				ioc->chains_needed_per_io,
				ioc->request_sz,
				sz / 1024);
	ioc->hi_priority =
	    ioc->request + ((ioc->scsiio_depth + 1) * ioc->request_sz);
	ioc->hi_priority_dma =
	    ioc->request_dma + ((ioc->scsiio_depth + 1) * ioc->request_sz);
	ioc->internal =
	    ioc->hi_priority + (ioc->hi_priority_depth * ioc->request_sz);
	ioc->internal_dma =
	    ioc->hi_priority_dma + (ioc->hi_priority_depth * ioc->request_sz);
	pr_info(
		"%s request pool(0x%p) - dma(0x%llx):\n\t\t"
		"depth(%d), frame_size(%d), pool_size(%d kB)\n",
			ioc->name,
			ioc->request,
			(unsigned long long)ioc->request_dma,
			ioc->hba_queue_depth,
			ioc->request_sz,
			(ioc->hba_queue_depth * ioc->request_sz) / 1024);
	total_sz += sz;
	ioc->io_queue_num = kcalloc(ioc->scsiio_depth, sizeof(u16), GFP_KERNEL);
	if (!ioc->io_queue_num) {
		rc = -ENOMEM;
		goto out;
	}
	dinitprintk(ioc, pr_info("%s scsiio(0x%p): depth(%d)\n",
		ioc->name, ioc->request, ioc->scsiio_depth));
	ioc->hpr_lookup = kcalloc(ioc->hi_priority_depth,
				  sizeof(struct leapioraid_request_tracker), GFP_KERNEL);
	if (!ioc->hpr_lookup) {
		rc = -ENOMEM;
		goto out;
	}
	ioc->hi_priority_smid = ioc->scsiio_depth + 1;
	dinitprintk(ioc, pr_info(
		"%s hi_priority(0x%p): depth(%d), start smid(%d)\n",
		ioc->name, ioc->hi_priority, ioc->hi_priority_depth,
		ioc->hi_priority_smid));
	ioc->internal_lookup =
	    kcalloc(ioc->internal_depth, sizeof(struct leapioraid_request_tracker),
		    GFP_KERNEL);
	if (!ioc->internal_lookup) {
		pr_err("%s internal_lookup: kcalloc failed\n",
		       ioc->name);
		rc = -ENOMEM;
		goto out;
	}
	ioc->internal_smid = ioc->hi_priority_smid + ioc->hi_priority_depth;
	dinitprintk(ioc, pr_info(
		"%s internal(0x%p): depth(%d), start smid(%d)\n",
		ioc->name, ioc->internal, ioc->internal_depth,
		ioc->internal_smid));
	sz = ioc->scsiio_depth * sizeof(struct leapioraid_chain_lookup);
	ioc->chain_lookup = kzalloc(sz, GFP_KERNEL);
	if (!ioc->chain_lookup) {
		if ((max_request_credit - 64) >
		    (ioc->internal_depth + LEAPIORAID_INTERNAL_SCSIIO_CMDS_COUNT)) {
			max_request_credit -= 64;
			leapioraid_base_release_memory_pools(ioc);
			goto retry;
		} else {
			pr_err(
			       "%s chain_lookup: __get_free_pages failed\n",
			       ioc->name);
			rc = -ENOMEM;
			goto out;
		}
	}
	sz = ioc->chains_needed_per_io * sizeof(struct leapioraid_chain_tracker);
	for (i = 0; i < ioc->scsiio_depth; i++) {
		ioc->chain_lookup[i].chains_per_smid = kzalloc(sz, GFP_KERNEL);
		if (!ioc->chain_lookup[i].chains_per_smid) {
			if ((max_request_credit - 64) >
			    (ioc->internal_depth +
			     LEAPIORAID_INTERNAL_SCSIIO_CMDS_COUNT)) {
				max_request_credit -= 64;
				leapioraid_base_release_memory_pools(ioc);
				goto retry;
			} else {
				pr_err("%s chain_lookup:  kzalloc failed\n", ioc->name);
				rc = -ENOMEM;
				goto out;
			}
		}
	}
	ioc->chains_per_prp_buffer = 0;
	rc = leapioraid_base_allocate_chain_dma_pool(ioc, ioc->chain_segment_sz);
	if (rc == -ENOMEM)
		return -ENOMEM;
	else if (rc == -EAGAIN) {
		if (ioc->use_32bit_dma && ioc->dma_mask > 32)
			goto try_32bit_dma;
		else {
			if ((max_request_credit - 64) >
			    (ioc->internal_depth +
			     LEAPIORAID_INTERNAL_SCSIIO_CMDS_COUNT)) {
				max_request_credit -= 64;
				leapioraid_base_release_memory_pools(ioc);
				goto retry_allocation;
			} else {
				pr_err("%s chain_lookup:  dma_pool_alloc failed\n", ioc->name);
				return -ENOMEM;
			}
		}
	}
	total_sz += ioc->chain_segment_sz *
	    ((ioc->chains_needed_per_io - ioc->chains_per_prp_buffer) *
	     ioc->scsiio_depth);
	sense_sz = ioc->scsiio_depth * SCSI_SENSE_BUFFERSIZE;
	rc = leapioraid_base_allocate_sense_dma_pool(ioc, sense_sz);
	if (rc == -ENOMEM)
		return -ENOMEM;
	else if (rc == -EAGAIN)
		goto try_32bit_dma;
	total_sz += sense_sz;
	sz = ioc->reply_free_queue_depth * ioc->reply_sz;
	rc = leapioraid_base_allocate_reply_pool(ioc, sz);
	if (rc == -ENOMEM)
		return -ENOMEM;
	else if (rc == -EAGAIN)
		goto try_32bit_dma;
	total_sz += sz;
	sz = ioc->reply_free_queue_depth * 4;
	rc = leapioraid_base_allocate_reply_free_dma_pool(ioc, sz);
	if (rc == -ENOMEM)
		return -ENOMEM;
	else if (rc == -EAGAIN)
		goto try_32bit_dma;
	total_sz += sz;
	reply_post_free_sz = ioc->reply_post_queue_depth *
	    sizeof(struct LeapioraidDefaultRepDesc_t);
	rdpq_sz = reply_post_free_sz * LEAPIORAID_RDPQ_MAX_INDEX_IN_ONE_CHUNK;
	if ((leapioraid_base_is_controller_msix_enabled(ioc)
	     && !ioc->rdpq_array_enable)
	    || (ioc->reply_queue_count < LEAPIORAID_RDPQ_MAX_INDEX_IN_ONE_CHUNK))
		rdpq_sz = reply_post_free_sz * ioc->reply_queue_count;
	rc = base_alloc_rdpq_dma_pool(ioc, rdpq_sz);
	if (rc == -ENOMEM)
		return -ENOMEM;
	else if (rc == -EAGAIN)
		goto try_32bit_dma;
	else {
		if (ioc->rdpq_array_enable && rc == 0) {
			reply_post_free_array_sz = ioc->reply_queue_count *
			    sizeof(struct LeapioraidIOCInitRDPQArrayEntry);
			rc = leapioraid_base_allocate_reply_post_free_array(
				ioc, reply_post_free_array_sz);
			if (rc == -ENOMEM)
				return -ENOMEM;
			else if (rc == -EAGAIN)
				goto try_32bit_dma;
		}
	}
	total_sz += rdpq_sz;
	ioc->config_page_sz = 512;
	ioc->config_page = dma_alloc_coherent(&ioc->pdev->dev,
					      ioc->config_page_sz,
					      &ioc->config_page_dma,
					      GFP_KERNEL);
	if (!ioc->config_page) {
		pr_err("%s config page: dma_pool_alloc failed\n", ioc->name);
		rc = -ENOMEM;
		goto out;
	}
	pr_err("%s config page(0x%p) - dma(0x%llx): size(%d)\n",
		ioc->name, ioc->config_page,
		(unsigned long long)ioc->config_page_dma,
		ioc->config_page_sz);
	total_sz += ioc->config_page_sz;
	pr_info("%s Allocated physical memory: size(%d kB)\n",
	       ioc->name, total_sz / 1024);
	pr_info(
		"%s Current IOC Queue Depth(%d), Max Queue Depth(%d)\n",
			ioc->name,
			ioc->shost->can_queue,
			facts->RequestCredit);
	return 0;
try_32bit_dma:
	leapioraid_base_release_memory_pools(ioc);
	if (ioc->use_32bit_dma && (ioc->dma_mask > 32)) {
		if (leapioraid_base_config_dma_addressing(ioc, ioc->pdev) != 0) {
			pr_err("Setting 32 bit coherent DMA mask Failed %s\n",
			       pci_name(ioc->pdev));
			return -ENODEV;
		}
	} else if (leapioraid_base_reduce_hba_queue_depth(ioc) != 0)
		return -ENOMEM;
	goto retry_allocation;
out:
	return rc;
}

static void
leapioraid_base_flush_ios_and_panic(
	struct LEAPIORAID_ADAPTER *ioc, u16 fault_code)
{
	ioc->adapter_over_temp = 1;
	leapioraid_base_stop_smart_polling(ioc);
	leapioraid_base_stop_watchdog(ioc);
	leapioraid_base_stop_hba_unplug_watchdog(ioc);
	leapioraid_base_pause_mq_polling(ioc);
	leapioraid_scsihost_flush_running_cmds(ioc);
	leapioraid_print_fault_code(ioc, fault_code);
}

u32
leapioraid_base_get_iocstate(struct LEAPIORAID_ADAPTER *ioc, int cooked)
{
	u32 s, sc;

	s = ioc->base_readl(
		&ioc->chip->Doorbell, LEAPIORAID_READL_RETRY_COUNT_OF_THIRTY);
	sc = s & LEAPIORAID_IOC_STATE_MASK;
	if (sc != LEAPIORAID_IOC_STATE_MASK) {
		if ((sc == LEAPIORAID_IOC_STATE_FAULT) &&
		    ((s & LEAPIORAID_DOORBELL_DATA_MASK) ==
		     LEAPIORAID_IFAULT_IOP_OVER_TEMP_THRESHOLD_EXCEEDED)) {
			leapioraid_base_flush_ios_and_panic(ioc,
						  s &
						  LEAPIORAID_DOORBELL_DATA_MASK);
			panic("TEMPERATURE FAULT: STOPPING; panic in %s\n",
			      __func__);
		}
	}
	return cooked ? sc : s;
}

static int
leapioraid_base_send_ioc_reset(
	struct LEAPIORAID_ADAPTER *ioc, u8 reset_type, int timeout)
{
	u32 ioc_state;
	int r = 0;
	unsigned long flags;

	if (reset_type != LEAPIORAID_FUNC_IOC_MESSAGE_UNIT_RESET) {
		pr_err("%s %s: unknown reset_type\n",
		       ioc->name, __func__);
		return -EFAULT;
	}
	if (!(ioc->facts.IOCCapabilities &
	      LEAPIORAID_IOCFACTS_CAPABILITY_EVENT_REPLAY))
		return -EFAULT;
	pr_info("%s sending message unit reset !!\n",
	       ioc->name);
	writel(reset_type << LEAPIORAID_DOORBELL_FUNCTION_SHIFT,
	       &ioc->chip->Doorbell);
	if ((leapioraid_base_wait_for_doorbell_ack(ioc, 15)))
		r = -EFAULT;
	ioc_state = leapioraid_base_get_iocstate(ioc, 0);
	spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock, flags);
	if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_COREDUMP
	    && (ioc->is_driver_loading == 1
		|| ioc->fault_reset_work_q == NULL)) {
		spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
		leapioraid_base_coredump_info(ioc, ioc_state);
		leapioraid_base_wait_for_coredump_completion(ioc, __func__);
		r = -EFAULT;
		goto out;
	}
	spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
	if (r != 0)
		goto out;
	ioc_state =
	    leapioraid_base_wait_on_iocstate(ioc, LEAPIORAID_IOC_STATE_READY,
					     timeout);
	if (ioc_state) {
		pr_err("%s %s: failed going to ready state (ioc_state=0x%x)\n",
			ioc->name, __func__, ioc_state);
		r = -EFAULT;
		goto out;
	}
out:
	pr_info("%s message unit reset: %s\n",
	       ioc->name, ((r == 0) ? "SUCCESS" : "FAILED"));
	return r;
}

int
leapioraid_wait_for_ioc_to_operational(struct LEAPIORAID_ADAPTER *ioc,
				       int wait_count)
{
	int wait_state_count = 0;
	u32 ioc_state;

	if (leapioraid_base_pci_device_is_unplugged(ioc))
		return -EFAULT;
	ioc_state = leapioraid_base_get_iocstate(ioc, 1);
	while (ioc_state != LEAPIORAID_IOC_STATE_OPERATIONAL) {
		if (leapioraid_base_pci_device_is_unplugged(ioc))
			return -EFAULT;
		if (ioc->is_driver_loading)
			return -ETIME;
		if (wait_state_count++ == wait_count) {
			pr_err(
			       "%s %s: failed due to ioc not operational\n",
			       ioc->name, __func__);
			return -EFAULT;
		}
		ssleep(1);
		ioc_state = leapioraid_base_get_iocstate(ioc, 1);
		pr_info("%s %s: waiting for operational state(count=%d)\n",
			ioc->name, __func__, wait_state_count);
	}
	if (wait_state_count)
		pr_info("%s %s: ioc is operational\n",
		       ioc->name, __func__);
	return 0;
}

int
leapioraid_base_sas_iounit_control(struct LEAPIORAID_ADAPTER *ioc,
				   struct LeapioraidSasIoUnitControlRep_t *mpi_reply,
				   struct LeapioraidSasIoUnitControlReq_t *mpi_request)
{
	u16 smid;
	u8 issue_reset;
	int rc;
	void *request;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	mutex_lock(&ioc->base_cmds.mutex);
	if (ioc->base_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_err("%s %s: base_cmd in use\n",
		       ioc->name, __func__);
		rc = -EAGAIN;
		goto out;
	}
	rc = leapioraid_wait_for_ioc_to_operational(ioc, 10);
	if (rc)
		goto out;
	smid = leapioraid_base_get_smid(ioc, ioc->base_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		rc = -EAGAIN;
		goto out;
	}
	rc = 0;
	ioc->base_cmds.status = LEAPIORAID_CMD_PENDING;
	request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->base_cmds.smid = smid;
	memcpy(request, mpi_request, sizeof(struct LeapioraidSasIoUnitControlReq_t));
	if (mpi_request->Operation == LEAPIORAID_SAS_OP_PHY_HARD_RESET ||
	    mpi_request->Operation == LEAPIORAID_SAS_OP_PHY_LINK_RESET)
		ioc->ioc_link_reset_in_progress = 1;
	init_completion(&ioc->base_cmds.done);
	ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->base_cmds.done,
				    msecs_to_jiffies(10000));
	if ((mpi_request->Operation == LEAPIORAID_SAS_OP_PHY_HARD_RESET ||
	     mpi_request->Operation == LEAPIORAID_SAS_OP_PHY_LINK_RESET) &&
	    ioc->ioc_link_reset_in_progress)
		ioc->ioc_link_reset_in_progress = 0;
	if (!(ioc->base_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		leapioraid_check_cmd_timeout(ioc,
					     ioc->base_cmds.status, mpi_request,
					     sizeof
					     (struct LeapioraidSasIoUnitControlReq_t)
					     / 4, issue_reset);
		goto issue_host_reset;
	}
	if (ioc->base_cmds.status & LEAPIORAID_CMD_REPLY_VALID)
		memcpy(mpi_reply, ioc->base_cmds.reply,
		       sizeof(struct LeapioraidSasIoUnitControlRep_t));
	else
		memset(mpi_reply, 0, sizeof(struct LeapioraidSasIoUnitControlRep_t));
	ioc->base_cmds.status = LEAPIORAID_CMD_NOT_USED;
	goto out;
issue_host_reset:
	if (issue_reset)
		leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
	ioc->base_cmds.status = LEAPIORAID_CMD_NOT_USED;
	rc = -EFAULT;
out:
	mutex_unlock(&ioc->base_cmds.mutex);
	return rc;
}

int
leapioraid_base_scsi_enclosure_processor(struct LEAPIORAID_ADAPTER *ioc,
					 struct LeapioraidSepRep_t *mpi_reply,
					 struct LeapioraidSepReq_t *mpi_request)
{
	u16 smid;
	u8 issue_reset;
	int rc;
	void *request;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	mutex_lock(&ioc->base_cmds.mutex);
	if (ioc->base_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_err("%s %s: base_cmd in use\n",
		       ioc->name, __func__);
		rc = -EAGAIN;
		goto out;
	}
	rc = leapioraid_wait_for_ioc_to_operational(ioc, 10);
	if (rc)
		goto out;
	smid = leapioraid_base_get_smid(ioc, ioc->base_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		rc = -EAGAIN;
		goto out;
	}
	rc = 0;
	ioc->base_cmds.status = LEAPIORAID_CMD_PENDING;
	request = leapioraid_base_get_msg_frame(ioc, smid);
	memset(request, 0, ioc->request_sz);
	ioc->base_cmds.smid = smid;
	memcpy(request, mpi_request, sizeof(struct LeapioraidSepReq_t));
	init_completion(&ioc->base_cmds.done);
	ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->base_cmds.done,
				    msecs_to_jiffies(10000));
	if (!(ioc->base_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		leapioraid_check_cmd_timeout(ioc,
					     ioc->base_cmds.status, mpi_request,
					     sizeof(struct LeapioraidSepReq_t) / 4,
					     issue_reset);
		goto issue_host_reset;
	}
	if (ioc->base_cmds.status & LEAPIORAID_CMD_REPLY_VALID)
		memcpy(mpi_reply, ioc->base_cmds.reply,
		       sizeof(struct LeapioraidSepRep_t));
	else
		memset(mpi_reply, 0, sizeof(struct LeapioraidSepRep_t));
	ioc->base_cmds.status = LEAPIORAID_CMD_NOT_USED;
	goto out;
issue_host_reset:
	if (issue_reset)
		leapioraid_base_hard_reset_handler(ioc, FORCE_BIG_HAMMER);
	ioc->base_cmds.status = LEAPIORAID_CMD_NOT_USED;
	rc = -EFAULT;
out:
	mutex_unlock(&ioc->base_cmds.mutex);
	return rc;
}

static int
leapioraid_base_get_port_facts(struct LEAPIORAID_ADAPTER *ioc, int port)
{
	struct LeapioraidPortFactsReq_t mpi_request;
	struct LeapioraidPortFactsRep_t mpi_reply;
	struct leapioraid_port_facts *pfacts;
	int mpi_reply_sz, mpi_request_sz, r;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	mpi_reply_sz = sizeof(struct LeapioraidPortFactsRep_t);
	mpi_request_sz = sizeof(struct LeapioraidPortFactsReq_t);
	memset(&mpi_request, 0, mpi_request_sz);
	mpi_request.Function = LEAPIORAID_FUNC_PORT_FACTS;
	mpi_request.PortNumber = port;
	r = leapioraid_base_handshake_req_reply_wait(ioc, mpi_request_sz,
						     (u32 *) &mpi_request,
						     mpi_reply_sz,
						     (u16 *) &mpi_reply, 5);
	if (r != 0) {
		pr_err("%s %s: handshake failed (r=%d)\n",
		       ioc->name, __func__, r);
		return r;
	}
	pfacts = &ioc->pfacts[port];
	memset(pfacts, 0, sizeof(struct leapioraid_port_facts));
	pfacts->PortNumber = mpi_reply.PortNumber;
	pfacts->VP_ID = mpi_reply.VP_ID;
	pfacts->VF_ID = mpi_reply.VF_ID;
	pfacts->MaxPostedCmdBuffers =
	    le16_to_cpu(mpi_reply.MaxPostedCmdBuffers);
	return 0;
}

static int
leapioraid_base_send_ioc_init(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidIOCInitReq_t mpi_request;
	struct LeapioraidIOCInitRep_t mpi_reply;
	int i, r = 0;
	ktime_t current_time;
	u16 ioc_status;
	u32 reply_post_free_ary_sz;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	memset(&mpi_request, 0, sizeof(struct LeapioraidIOCInitReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_IOC_INIT;
	mpi_request.WhoInit = LEAPIORAID_WHOINIT_HOST_DRIVER;
	mpi_request.VF_ID = 0;
	mpi_request.VP_ID = 0;
	mpi_request.MsgVersion = cpu_to_le16(0x0206);
	mpi_request.HeaderVersion = cpu_to_le16(0x3A00);
	mpi_request.HostPageSize = 12;
	if (leapioraid_base_is_controller_msix_enabled(ioc))
		mpi_request.HostMSIxVectors = ioc->reply_queue_count;
	mpi_request.SystemRequestFrameSize = cpu_to_le16(ioc->request_sz / 4);
	mpi_request.ReplyDescriptorPostQueueDepth =
	    cpu_to_le16(ioc->reply_post_queue_depth);
	mpi_request.ReplyFreeQueueDepth =
	    cpu_to_le16(ioc->reply_free_queue_depth);
	mpi_request.SenseBufferAddressHigh =
	    cpu_to_le32((u64) ioc->sense_dma >> 32);
	mpi_request.SystemReplyAddressHigh =
	    cpu_to_le32((u64) ioc->reply_dma >> 32);
	mpi_request.SystemRequestFrameBaseAddress =
	    cpu_to_le64((u64) ioc->request_dma);
	mpi_request.ReplyFreeQueueAddress =
	    cpu_to_le64((u64) ioc->reply_free_dma);
	if (ioc->rdpq_array_enable) {
		reply_post_free_ary_sz = ioc->reply_queue_count *
		    sizeof(struct LeapioraidIOCInitRDPQArrayEntry);
		memset(ioc->reply_post_free_array, 0, reply_post_free_ary_sz);
		for (i = 0; i < ioc->reply_queue_count; i++)
			ioc->reply_post_free_array[i].RDPQBaseAddress =
			    cpu_to_le64((u64) ioc->reply_post[i].reply_post_free_dma);
		mpi_request.MsgFlags = LEAPIORAID_IOCINIT_MSGFLAG_RDPQ_ARRAY_MODE;
		mpi_request.ReplyDescriptorPostQueueAddress =
		    cpu_to_le64((u64) ioc->reply_post_free_array_dma);
	} else {
		mpi_request.ReplyDescriptorPostQueueAddress =
		    cpu_to_le64((u64) ioc->reply_post[0].reply_post_free_dma);
	}
	mpi_request.ConfigurationFlags |= 0x0002;
	current_time = ktime_get_real();
	mpi_request.TimeStamp = cpu_to_le64(ktime_to_ms(current_time));
	if (ioc->logging_level & LEAPIORAID_DEBUG_INIT) {

		pr_info("%s \toffset:data\n", ioc->name);
		leapioraid_debug_dump_mf(&mpi_request,
				sizeof(struct LeapioraidIOCInitReq_t) / 4);

	}
	r = leapioraid_base_handshake_req_reply_wait(ioc,
						     sizeof
						     (struct LeapioraidIOCInitReq_t),
						     (u32 *) &mpi_request,
						     sizeof
						     (struct LeapioraidIOCInitRep_t),
						     (u16 *) &mpi_reply, 30);
	if (r != 0) {
		pr_err("%s %s: handshake failed (r=%d)\n",
		       ioc->name, __func__, r);
		return r;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS || mpi_reply.IOCLogInfo) {
		pr_err("%s %s: failed\n", ioc->name,
		       __func__);
		r = -EIO;
	}
	ioc->timestamp_update_count = 0;
	return r;
}

int
leapioraid_base_trace_log_init(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidIOCLogReq_t mpi_request;
	struct LeapioraidIOCLogRep_t mpi_reply;
	u16 ioc_status;
	u32 r;

	dinitprintk(ioc,
		    pr_info("%s %s\n", ioc->name, __func__));
	if (ioc->log_buffer == NULL) {
		ioc->log_buffer =
			dma_alloc_coherent(&ioc->pdev->dev,
				 (SYS_LOG_BUF_SIZE + SYS_LOG_BUF_RESERVE),
				 &ioc->log_buffer_dma, GFP_KERNEL);
		if (!ioc->log_buffer) {
			pr_err("%s: Failed to allocate log_buffer at %s:%d/%s()!\n",
				ioc->name, __FILE__, __LINE__, __func__);
			return -ENOMEM;
		}

	}
	memset(ioc->log_buffer, 0, (SYS_LOG_BUF_SIZE + SYS_LOG_BUF_RESERVE));

	memset(&mpi_request, 0, sizeof(struct LeapioraidIOCLogReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_LOG_INIT;
	mpi_request.BufAddr = ioc->log_buffer_dma;
	mpi_request.BufSize = SYS_LOG_BUF_SIZE;
	r = leapioraid_base_handshake_req_reply_wait(ioc,
						     sizeof
						     (struct LeapioraidIOCLogReq_t),
						     (u32 *) &mpi_request,
						     sizeof
						     (struct LeapioraidIOCLogRep_t),
						     (u16 *) &mpi_reply, 30);
	if (r != 0) {
		pr_err("%s %s: handshake failed (r=%d)\n",
		       ioc->name, __func__, r);
		return r;
	}
	ioc_status = le16_to_cpu(mpi_reply.IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS || mpi_reply.IOCLogInfo) {
		pr_err("%s %s: failed\n", ioc->name,
		       __func__);
		r = -EIO;
	}
	return r;
}

static int
leapioraid_base_trace_log_exit(struct LEAPIORAID_ADAPTER *ioc)
{
	if (ioc->log_buffer)
		dma_free_coherent(&ioc->pdev->dev, SYS_LOG_BUF_SIZE,
				  ioc->log_buffer, ioc->log_buffer_dma);
	return 0;
}

u8
leapioraid_port_enable_done(struct LEAPIORAID_ADAPTER *ioc, u16 smid,
			    u8 msix_index, u32 reply)
{
	struct LeapioraidDefaultRep_t *mpi_reply;
	u16 ioc_status;

	if (ioc->port_enable_cmds.status == LEAPIORAID_CMD_NOT_USED)
		return 1;
	mpi_reply = leapioraid_base_get_reply_virt_addr(ioc, reply);
	if (!mpi_reply)
		return 1;
	if (mpi_reply->Function != LEAPIORAID_FUNC_PORT_ENABLE)
		return 1;
	ioc->port_enable_cmds.status &= ~LEAPIORAID_CMD_PENDING;
	ioc->port_enable_cmds.status |= LEAPIORAID_CMD_COMPLETE;
	ioc->port_enable_cmds.status |= LEAPIORAID_CMD_REPLY_VALID;
	memcpy(ioc->port_enable_cmds.reply, mpi_reply,
	       mpi_reply->MsgLength * 4);
	ioc_status = le16_to_cpu(mpi_reply->IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS)
		ioc->port_enable_failed = 1;
	if (ioc->port_enable_cmds.status & LEAPIORAID_CMD_COMPLETE_ASYNC) {
		ioc->port_enable_cmds.status &= ~LEAPIORAID_CMD_COMPLETE_ASYNC;
		if (ioc_status == LEAPIORAID_IOCSTATUS_SUCCESS) {
			leapioraid_port_enable_complete(ioc);
			return 1;
		}

		ioc->start_scan_failed = ioc_status;
		ioc->start_scan = 0;
		return 1;
	}
	complete(&ioc->port_enable_cmds.done);
	return 1;
}

static int
leapioraid_base_send_port_enable(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidPortEnableReq_t *mpi_request;
	struct LeapioraidPortEnableRep_t *mpi_reply;
	int r = 0;
	u16 smid;
	u16 ioc_status;

	pr_info("%s sending port enable !!\n", ioc->name);
	if (ioc->port_enable_cmds.status & LEAPIORAID_CMD_PENDING) {
		pr_err(
		       "%s %s: internal command already in use\n", ioc->name,
		       __func__);
		return -EAGAIN;
	}
	smid = leapioraid_base_get_smid(ioc, ioc->port_enable_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		return -EAGAIN;
	}
	ioc->port_enable_cmds.status = LEAPIORAID_CMD_PENDING;
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->port_enable_cmds.smid = smid;
	memset(mpi_request, 0, sizeof(struct LeapioraidPortEnableReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_PORT_ENABLE;
	init_completion(&ioc->port_enable_cmds.done);
	ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->port_enable_cmds.done, 300 * HZ);
	if (!(ioc->port_enable_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		pr_err("%s %s: timeout\n",
		       ioc->name, __func__);
		leapioraid_debug_dump_mf(mpi_request,
			       sizeof(struct LeapioraidPortEnableReq_t) / 4);
		if (ioc->port_enable_cmds.status & LEAPIORAID_CMD_RESET)
			r = -EFAULT;
		else
			r = -ETIME;
		goto out;
	}
	mpi_reply = ioc->port_enable_cmds.reply;
	ioc_status = le16_to_cpu(mpi_reply->IOCStatus) & LEAPIORAID_IOCSTATUS_MASK;
	if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS) {
		pr_err(
		       "%s %s: failed with (ioc_status=0x%08x)\n", ioc->name,
		       __func__, ioc_status);
		r = -EFAULT;
		goto out;
	}
out:
	ioc->port_enable_cmds.status = LEAPIORAID_CMD_NOT_USED;
	pr_info("%s port enable: %s\n", ioc->name, ((r == 0) ?
								      "SUCCESS"
								      :
								      "FAILED"));
	return r;
}

int
leapioraid_port_enable(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidPortEnableReq_t *mpi_request;
	u16 smid;

	pr_info("%s sending port enable !!\n", ioc->name);
	if (ioc->port_enable_cmds.status & LEAPIORAID_CMD_PENDING) {
		pr_err(
		       "%s %s: internal command already in use\n", ioc->name,
		       __func__);
		return -EAGAIN;
	}
	smid = leapioraid_base_get_smid(ioc, ioc->port_enable_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		return -EAGAIN;
	}
	ioc->drv_internal_flags |= LEAPIORAID_DRV_INERNAL_FIRST_PE_ISSUED;
	ioc->port_enable_cmds.status = LEAPIORAID_CMD_PENDING;
	ioc->port_enable_cmds.status |= LEAPIORAID_CMD_COMPLETE_ASYNC;
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->port_enable_cmds.smid = smid;
	memset(mpi_request, 0, sizeof(struct LeapioraidPortEnableReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_PORT_ENABLE;
	ioc->put_smid_default(ioc, smid);
	return 0;
}

static int
leapioraid_base_determine_wait_on_discovery(struct LEAPIORAID_ADAPTER *ioc)
{
	if (ioc->ir_firmware)
		return 1;
	if (!ioc->bios_pg3.BiosVersion)
		return 0;
	if ((ioc->bios_pg2.CurrentBootDeviceForm &
	     LEAPIORAID_BIOSPAGE2_FORM_MASK) ==
	    LEAPIORAID_BIOSPAGE2_FORM_NO_DEVICE_SPECIFIED &&
	    (ioc->bios_pg2.ReqBootDeviceForm &
	     LEAPIORAID_BIOSPAGE2_FORM_MASK) ==
	    LEAPIORAID_BIOSPAGE2_FORM_NO_DEVICE_SPECIFIED &&
	    (ioc->bios_pg2.ReqAltBootDeviceForm &
	     LEAPIORAID_BIOSPAGE2_FORM_MASK) ==
	    LEAPIORAID_BIOSPAGE2_FORM_NO_DEVICE_SPECIFIED)
		return 0;
	return 1;
}

static void
leapioraid_base_unmask_events(struct LEAPIORAID_ADAPTER *ioc, u16 event)
{
	u32 desired_event;

	if (event >= 128)
		return;
	desired_event = (1 << (event % 32));
	if (event < 32)
		ioc->event_masks[0] &= ~desired_event;
	else if (event < 64)
		ioc->event_masks[1] &= ~desired_event;
	else if (event < 96)
		ioc->event_masks[2] &= ~desired_event;
	else if (event < 128)
		ioc->event_masks[3] &= ~desired_event;
}

static int
leapioraid_base_event_notification(struct LEAPIORAID_ADAPTER *ioc)
{
	struct LeapioraidEventNotificationReq_t *mpi_request;
	u16 smid;
	int r = 0;
	int i, issue_diag_reset = 0;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	if (ioc->base_cmds.status & LEAPIORAID_CMD_PENDING) {
		pr_err(
		       "%s %s: internal command already in use\n", ioc->name,
		       __func__);
		return -EAGAIN;
	}
	smid = leapioraid_base_get_smid(ioc, ioc->base_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		return -EAGAIN;
	}
	ioc->base_cmds.status = LEAPIORAID_CMD_PENDING;
	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->base_cmds.smid = smid;
	memset(mpi_request, 0, sizeof(struct LeapioraidEventNotificationReq_t));
	mpi_request->Function = LEAPIORAID_FUNC_EVENT_NOTIFICATION;
	mpi_request->VF_ID = 0;
	mpi_request->VP_ID = 0;
	for (i = 0; i < LEAPIORAID_EVENT_NOTIFY_EVENTMASK_WORDS; i++)
		mpi_request->EventMasks[i] = cpu_to_le32(ioc->event_masks[i]);
	init_completion(&ioc->base_cmds.done);
	ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->base_cmds.done, 30 * HZ);
	if (!(ioc->base_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		pr_err("%s %s: timeout\n",
		       ioc->name, __func__);
		leapioraid_debug_dump_mf(mpi_request,
			       sizeof(struct LeapioraidEventNotificationReq_t) / 4);
		if (ioc->base_cmds.status & LEAPIORAID_CMD_RESET)
			r = -EFAULT;
		else
			issue_diag_reset = 1;
	} else
		dinitprintk(ioc, pr_info("%s %s: complete\n",
					ioc->name, __func__));
	ioc->base_cmds.status = LEAPIORAID_CMD_NOT_USED;
	if (issue_diag_reset) {
		if (ioc->drv_internal_flags & LEAPIORAID_DRV_INERNAL_FIRST_PE_ISSUED)
			return -EFAULT;
		if (leapioraid_base_check_for_fault_and_issue_reset(ioc))
			return -EFAULT;
		r = -EAGAIN;
	}
	return r;
}

void
leapioraid_base_validate_event_type(struct LEAPIORAID_ADAPTER *ioc,
				    u32 *event_type)
{
	int i, j;
	u32 event_mask, desired_event;
	u8 send_update_to_fw;

	for (i = 0, send_update_to_fw = 0; i <
	     LEAPIORAID_EVENT_NOTIFY_EVENTMASK_WORDS; i++) {
		event_mask = ~event_type[i];
		desired_event = 1;
		for (j = 0; j < 32; j++) {
			if (!(event_mask & desired_event) &&
			    (ioc->event_masks[i] & desired_event)) {
				ioc->event_masks[i] &= ~desired_event;
				send_update_to_fw = 1;
			}
			desired_event = (desired_event << 1);
		}
	}
	if (!send_update_to_fw)
		return;
	mutex_lock(&ioc->base_cmds.mutex);
	leapioraid_base_event_notification(ioc);
	mutex_unlock(&ioc->base_cmds.mutex);
}

int
leapioraid_base_make_ioc_ready(struct LEAPIORAID_ADAPTER *ioc,
			       enum reset_type type)
{
	u32 ioc_state;
	int rc;
	int count;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	if (!leapioraid_base_pci_device_is_available(ioc))
		return 0;
	ioc_state = leapioraid_base_get_iocstate(ioc, 0);
	dhsprintk(ioc, pr_info("%s %s: ioc_state(0x%08x)\n",
			      ioc->name, __func__, ioc_state));
	count = 0;
	if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_RESET) {
		while ((ioc_state & LEAPIORAID_IOC_STATE_MASK) !=
		       LEAPIORAID_IOC_STATE_READY) {
			if (count++ == 10) {
				pr_err(
				       "%s %s: failed going to ready state (ioc_state=0x%x)\n",
				       ioc->name, __func__, ioc_state);
				return -EFAULT;
			}
			ssleep(1);
			ioc_state = leapioraid_base_get_iocstate(ioc, 0);
		}
	}
	if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_READY)
		return 0;
	if (ioc_state & LEAPIORAID_DOORBELL_USED) {
		pr_info("%s unexpected doorbell active!\n",
		       ioc->name);
		goto issue_diag_reset;
	}
	if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_FAULT) {
		leapioraid_print_fault_code(ioc, ioc_state &
					    LEAPIORAID_DOORBELL_DATA_MASK);
		goto issue_diag_reset;
	}
	if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) == LEAPIORAID_IOC_STATE_COREDUMP) {
		if (ioc->ioc_coredump_loop != 0xFF) {
			leapioraid_base_coredump_info(ioc, ioc_state &
						      LEAPIORAID_DOORBELL_DATA_MASK);
			leapioraid_base_wait_for_coredump_completion(ioc,
								     __func__);
		}
		goto issue_diag_reset;
	}
	if (type == FORCE_BIG_HAMMER)
		goto issue_diag_reset;
	if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) ==
	    LEAPIORAID_IOC_STATE_OPERATIONAL)
		if (!
		    (leapioraid_base_send_ioc_reset
		     (ioc, LEAPIORAID_FUNC_IOC_MESSAGE_UNIT_RESET, 15))) {
			return 0;
		}
issue_diag_reset:
	rc = leapioraid_base_diag_reset(ioc);
	return rc;
}

static int
leapioraid_base_make_ioc_operational(struct LEAPIORAID_ADAPTER *ioc)
{
	int r, rc, i, index;
	unsigned long flags;
	u32 reply_address;
	u16 smid;
	struct leapioraid_tr_list *delayed_tr, *delayed_tr_next;
	struct leapioraid_sc_list *delayed_sc, *delayed_sc_next;
	struct leapioraid_event_ack_list *delayed_event_ack, *delayed_event_ack_next;
	struct leapioraid_adapter_reply_queue *reply_q;
	union LeapioraidRepDescUnion_t *reply_post_free_contig;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	list_for_each_entry_safe(delayed_tr, delayed_tr_next,
				 &ioc->delayed_tr_list, list) {
		list_del(&delayed_tr->list);
		kfree(delayed_tr);
	}
	list_for_each_entry_safe(delayed_tr, delayed_tr_next,
				 &ioc->delayed_tr_volume_list, list) {
		list_del(&delayed_tr->list);
		kfree(delayed_tr);
	}
	list_for_each_entry_safe(delayed_tr, delayed_tr_next,
				 &ioc->delayed_internal_tm_list, list) {
		list_del(&delayed_tr->list);
		kfree(delayed_tr);
	}
	list_for_each_entry_safe(delayed_sc, delayed_sc_next,
				 &ioc->delayed_sc_list, list) {
		list_del(&delayed_sc->list);
		kfree(delayed_sc);
	}
	list_for_each_entry_safe(delayed_event_ack, delayed_event_ack_next,
				 &ioc->delayed_event_ack_list, list) {
		list_del(&delayed_event_ack->list);
		kfree(delayed_event_ack);
	}
	spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
	INIT_LIST_HEAD(&ioc->hpr_free_list);
	smid = ioc->hi_priority_smid;
	for (i = 0; i < ioc->hi_priority_depth; i++, smid++) {
		ioc->hpr_lookup[i].cb_idx = 0xFF;
		ioc->hpr_lookup[i].smid = smid;
		list_add_tail(&ioc->hpr_lookup[i].tracker_list,
			      &ioc->hpr_free_list);
	}
	INIT_LIST_HEAD(&ioc->internal_free_list);
	smid = ioc->internal_smid;
	for (i = 0; i < ioc->internal_depth; i++, smid++) {
		ioc->internal_lookup[i].cb_idx = 0xFF;
		ioc->internal_lookup[i].smid = smid;
		list_add_tail(&ioc->internal_lookup[i].tracker_list,
			      &ioc->internal_free_list);
	}
	spin_unlock_irqrestore(&ioc->scsi_lookup_lock, flags);
	for (i = 0, reply_address = (u32) ioc->reply_dma;
	     i < ioc->reply_free_queue_depth; i++, reply_address +=
	     ioc->reply_sz) {
		ioc->reply_free[i] = cpu_to_le32(reply_address);
	}
	if (ioc->is_driver_loading)
		leapioraid_base_assign_reply_queues(ioc);
	index = 0;
	reply_post_free_contig = ioc->reply_post[0].reply_post_free;
	list_for_each_entry(reply_q, &ioc->reply_queue_list, list) {
		if (ioc->rdpq_array_enable) {
			reply_q->reply_post_free =
			    ioc->reply_post[index++].reply_post_free;
		} else {
			reply_q->reply_post_free = reply_post_free_contig;
			reply_post_free_contig += ioc->reply_post_queue_depth;
		}
		reply_q->reply_post_host_index = 0;
		for (i = 0; i < ioc->reply_post_queue_depth; i++)
			reply_q->reply_post_free[i].Words =
			    cpu_to_le64(ULLONG_MAX);
		if (!leapioraid_base_is_controller_msix_enabled(ioc))
			goto skip_init_reply_post_free_queue;
	}
skip_init_reply_post_free_queue:
	r = leapioraid_base_send_ioc_init(ioc);
	if (r) {
		if (!ioc->is_driver_loading)
			return r;
		rc = leapioraid_base_check_for_fault_and_issue_reset(ioc);
		if (rc || (leapioraid_base_send_ioc_init(ioc)))
			return r;
	}
	ioc->reply_free_host_index = ioc->reply_free_queue_depth - 1;
	writel(ioc->reply_free_host_index, &ioc->chip->ReplyFreeHostIndex);
	list_for_each_entry(reply_q, &ioc->reply_queue_list, list) {
		if (ioc->combined_reply_queue) {
			for (i = 0; i < ioc->nc_reply_index_count; i++)
				writel((reply_q->msix_index & 7) <<
				       LEAPIORAID_RPHI_MSIX_INDEX_SHIFT,
				       ioc->replyPostRegisterIndex[i]);
		} else {
			writel(reply_q->msix_index << LEAPIORAID_RPHI_MSIX_INDEX_SHIFT,
			       &ioc->chip->ReplyPostHostIndex);
		}
		if (!leapioraid_base_is_controller_msix_enabled(ioc))
			goto skip_init_reply_post_host_index;
	}
skip_init_reply_post_host_index:
	leapioraid_base_unmask_interrupts(ioc);
	r = leapioraid_base_display_fwpkg_version(ioc);
	if (r)
		return r;
	r = leapioraid_base_static_config_pages(ioc);
	if (r)
		return r;
	r = leapioraid_base_event_notification(ioc);
	if (r)
		return r;
	leapioraid_base_start_hba_unplug_watchdog(ioc);
	if (!ioc->shost_recovery) {
		ioc->wait_for_discovery_to_complete =
		    leapioraid_base_determine_wait_on_discovery(ioc);
		return r;
	}
	r = leapioraid_base_send_port_enable(ioc);
	if (r)
		return r;
	return r;
}

void
leapioraid_base_free_resources(struct LEAPIORAID_ADAPTER *ioc)
{
	dexitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	if (!ioc->chip_phys)
		return;
	leapioraid_base_mask_interrupts(ioc);
	ioc->shost_recovery = 1;
	leapioraid_base_make_ioc_ready(ioc, SOFT_RESET);
	ioc->shost_recovery = 0;
	leapioraid_base_unmap_resources(ioc);
}

int
leapioraid_base_attach(struct LEAPIORAID_ADAPTER *ioc)
{
	int r, rc, i;
	int cpu_id, last_cpu_id = 0;

	dinitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	ioc->cpu_count = num_online_cpus();
	for_each_online_cpu(cpu_id)
		last_cpu_id = cpu_id;
	ioc->cpu_msix_table_sz = last_cpu_id + 1;
	ioc->cpu_msix_table = kzalloc(ioc->cpu_msix_table_sz, GFP_KERNEL);
	ioc->reply_queue_count = 1;
	if (!ioc->cpu_msix_table) {
		r = -ENOMEM;
		goto out_free_resources;
	}
	ioc->rdpq_array_enable_assigned = 0;
	ioc->use_32bit_dma = 0;
	ioc->dma_mask = 64;
	ioc->base_readl = &leapioraid_base_readl_aero;
	ioc->smp_affinity_enable = smp_affinity_enable;
	r = leapioraid_base_map_resources(ioc);
	if (r)
		goto out_free_resources;
	pci_set_drvdata(ioc->pdev, ioc->shost);
	r = leapioraid_base_get_ioc_facts(ioc);
	if (r) {
		rc = leapioraid_base_check_for_fault_and_issue_reset(ioc);
		if (rc || (leapioraid_base_get_ioc_facts(ioc)))
			goto out_free_resources;
	}

	ioc->build_sg_scmd = &leapioraid_base_build_sg_scmd_ieee;
	ioc->build_sg = &leapioraid_base_build_sg_ieee;
	ioc->build_zero_len_sge =
		&leapioraid_base_build_zero_len_sge_ieee;
	ioc->sge_size_ieee = sizeof(struct LEAPIORAID_IEEE_SGE_SIMPLE64);
	if (ioc->high_iops_queues)
		ioc->get_msix_index_for_smlio =
			&leapioraid_base_get_high_iops_msix_index;
	else
		ioc->get_msix_index_for_smlio = &leapioraid_base_get_msix_index;

	if (ioc->atomic_desc_capable) {
		ioc->put_smid_default =
		    &leapioraid_base_put_smid_default_atomic;
		ioc->put_smid_scsi_io =
		    &leapioraid_base_put_smid_scsi_io_atomic;
		ioc->put_smid_fast_path =
		    &leapioraid_base_put_smid_fast_path_atomic;
		ioc->put_smid_hi_priority =
		    &leapioraid_base_put_smid_hi_priority_atomic;
	} else {
		ioc->put_smid_default = &leapioraid_base_put_smid_default;
		ioc->put_smid_scsi_io = &leapioraid_base_put_smid_scsi_io;
		ioc->put_smid_fast_path = &leapioraid_base_put_smid_fast_path;
		ioc->put_smid_hi_priority =
		    &leapioraid_base_put_smid_hi_priority;
	}
	ioc->build_sg_mpi = &leapioraid_base_build_sg;
	ioc->build_zero_len_sge_mpi = &leapioraid_base_build_zero_len_sge;
	r = leapioraid_base_make_ioc_ready(ioc, SOFT_RESET);
	if (r)
		goto out_free_resources;
	if (ioc->open_pcie_trace) {
		r = leapioraid_base_trace_log_init(ioc);
		if (r) {
			pr_err("log init failed\n");
			goto out_free_resources;
		}
	}
	ioc->pfacts = kcalloc(ioc->facts.NumberOfPorts,
			      sizeof(struct leapioraid_port_facts), GFP_KERNEL);
	if (!ioc->pfacts) {
		r = -ENOMEM;
		goto out_free_resources;
	}
	for (i = 0; i < ioc->facts.NumberOfPorts; i++) {
		r = leapioraid_base_get_port_facts(ioc, i);
		if (r) {
			rc = leapioraid_base_check_for_fault_and_issue_reset
			    (ioc);
			if (rc || (leapioraid_base_get_port_facts(ioc, i)))
				goto out_free_resources;
		}
	}
	r = leapioraid_base_allocate_memory_pools(ioc);
	if (r)
		goto out_free_resources;
	if (irqpoll_weight > 0)
		ioc->thresh_hold = irqpoll_weight;
	else
		ioc->thresh_hold = ioc->hba_queue_depth / 4;
	leapioraid_base_init_irqpolls(ioc);
	init_waitqueue_head(&ioc->reset_wq);
	ioc->pd_handles_sz = (ioc->facts.MaxDevHandle / 8);
	if (ioc->facts.MaxDevHandle % 8)
		ioc->pd_handles_sz++;
	ioc->pd_handles = kzalloc(ioc->pd_handles_sz, GFP_KERNEL);
	if (!ioc->pd_handles) {
		r = -ENOMEM;
		goto out_free_resources;
	}
	ioc->blocking_handles = kzalloc(ioc->pd_handles_sz, GFP_KERNEL);
	if (!ioc->blocking_handles) {
		r = -ENOMEM;
		goto out_free_resources;
	}
	ioc->pend_os_device_add_sz = (ioc->facts.MaxDevHandle / 8);
	if (ioc->facts.MaxDevHandle % 8)
		ioc->pend_os_device_add_sz++;
	ioc->pend_os_device_add = kzalloc(ioc->pend_os_device_add_sz,
					  GFP_KERNEL);
	if (!ioc->pend_os_device_add)
		goto out_free_resources;
	ioc->device_remove_in_progress_sz = ioc->pend_os_device_add_sz;
	ioc->device_remove_in_progress =
	    kzalloc(ioc->device_remove_in_progress_sz, GFP_KERNEL);
	if (!ioc->device_remove_in_progress)
		goto out_free_resources;
	ioc->tm_tr_retry_sz = ioc->facts.MaxDevHandle * sizeof(u8);
	ioc->tm_tr_retry = kzalloc(ioc->tm_tr_retry_sz, GFP_KERNEL);
	if (!ioc->tm_tr_retry)
		goto out_free_resources;
	ioc->fwfault_debug = leapioraid_fwfault_debug;
	mutex_init(&ioc->base_cmds.mutex);
	ioc->base_cmds.reply = kzalloc(ioc->reply_sz, GFP_KERNEL);
	ioc->base_cmds.status = LEAPIORAID_CMD_NOT_USED;
	ioc->port_enable_cmds.reply = kzalloc(ioc->reply_sz, GFP_KERNEL);
	ioc->port_enable_cmds.status = LEAPIORAID_CMD_NOT_USED;
	ioc->transport_cmds.reply = kzalloc(ioc->reply_sz, GFP_KERNEL);
	ioc->transport_cmds.status = LEAPIORAID_CMD_NOT_USED;
	mutex_init(&ioc->transport_cmds.mutex);
	ioc->scsih_cmds.reply = kzalloc(ioc->reply_sz, GFP_KERNEL);
	ioc->scsih_cmds.status = LEAPIORAID_CMD_NOT_USED;
	mutex_init(&ioc->scsih_cmds.mutex);
	ioc->tm_cmds.reply = kzalloc(ioc->reply_sz, GFP_KERNEL);
	ioc->tm_cmds.status = LEAPIORAID_CMD_NOT_USED;
	mutex_init(&ioc->tm_cmds.mutex);
	ioc->config_cmds.reply = kzalloc(ioc->reply_sz, GFP_KERNEL);
	ioc->config_cmds.status = LEAPIORAID_CMD_NOT_USED;
	mutex_init(&ioc->config_cmds.mutex);
	ioc->ctl_cmds.reply = kzalloc(ioc->reply_sz, GFP_KERNEL);
	ioc->ctl_cmds.sense = kzalloc(SCSI_SENSE_BUFFERSIZE, GFP_KERNEL);
	ioc->ctl_cmds.status = LEAPIORAID_CMD_NOT_USED;
	mutex_init(&ioc->ctl_cmds.mutex);

	if (!ioc->base_cmds.reply || !ioc->port_enable_cmds.reply ||
	    !ioc->transport_cmds.reply || !ioc->scsih_cmds.reply ||
	    !ioc->tm_cmds.reply || !ioc->config_cmds.reply ||
	    !ioc->ctl_cmds.reply || !ioc->ctl_cmds.sense) {
		r = -ENOMEM;
		goto out_free_resources;
	}
	for (i = 0; i < LEAPIORAID_EVENT_NOTIFY_EVENTMASK_WORDS; i++)
		ioc->event_masks[i] = -1;
	leapioraid_base_unmask_events(ioc, LEAPIORAID_EVENT_SAS_DISCOVERY);
	leapioraid_base_unmask_events(ioc,
				      LEAPIORAID_EVENT_SAS_BROADCAST_PRIMITIVE);
	leapioraid_base_unmask_events(ioc,
				      LEAPIORAID_EVENT_SAS_TOPOLOGY_CHANGE_LIST);
	leapioraid_base_unmask_events(ioc,
				      LEAPIORAID_EVENT_SAS_DEVICE_STATUS_CHANGE);
	leapioraid_base_unmask_events(ioc,
				      LEAPIORAID_EVENT_SAS_ENCL_DEVICE_STATUS_CHANGE);
	leapioraid_base_unmask_events(ioc,
				      LEAPIORAID_EVENT_IR_CONFIGURATION_CHANGE_LIST);
	leapioraid_base_unmask_events(ioc, LEAPIORAID_EVENT_IR_VOLUME);
	leapioraid_base_unmask_events(ioc, LEAPIORAID_EVENT_IR_PHYSICAL_DISK);
	leapioraid_base_unmask_events(ioc, LEAPIORAID_EVENT_IR_OPERATION_STATUS);
	leapioraid_base_unmask_events(ioc, LEAPIORAID_EVENT_LOG_ENTRY_ADDED);
	leapioraid_base_unmask_events(ioc, LEAPIORAID_EVENT_TEMP_THRESHOLD);
	leapioraid_base_unmask_events(ioc,
				      LEAPIORAID_EVENT_SAS_DEVICE_DISCOVERY_ERROR);
	r = leapioraid_base_make_ioc_operational(ioc);
	if (r == -EAGAIN)
		r = leapioraid_base_make_ioc_operational(ioc);
	if (r)
		goto out_free_resources;
	memcpy(&ioc->prev_fw_facts, &ioc->facts,
	       sizeof(struct leapioraid_facts));
	ioc->non_operational_loop = 0;
	ioc->ioc_coredump_loop = 0;
	ioc->got_task_abort_from_ioctl = 0;
	ioc->got_task_abort_from_sysfs = 0;
	return 0;
out_free_resources:
	ioc->remove_host = 1;
	leapioraid_base_free_resources(ioc);
	leapioraid_base_release_memory_pools(ioc);
	pci_set_drvdata(ioc->pdev, NULL);
	kfree(ioc->cpu_msix_table);
	kfree(ioc->pd_handles);
	kfree(ioc->blocking_handles);
	kfree(ioc->tm_tr_retry);
	kfree(ioc->device_remove_in_progress);
	kfree(ioc->pend_os_device_add);
	kfree(ioc->tm_cmds.reply);
	kfree(ioc->transport_cmds.reply);
	kfree(ioc->scsih_cmds.reply);
	kfree(ioc->config_cmds.reply);
	kfree(ioc->base_cmds.reply);
	kfree(ioc->port_enable_cmds.reply);
	kfree(ioc->ctl_cmds.reply);
	kfree(ioc->ctl_cmds.sense);
	kfree(ioc->pfacts);
	ioc->ctl_cmds.reply = NULL;
	ioc->base_cmds.reply = NULL;
	ioc->tm_cmds.reply = NULL;
	ioc->scsih_cmds.reply = NULL;
	ioc->transport_cmds.reply = NULL;
	ioc->config_cmds.reply = NULL;
	ioc->pfacts = NULL;
	return r;
}

void
leapioraid_base_detach(struct LEAPIORAID_ADAPTER *ioc)
{
	dexitprintk(ioc, pr_info("%s %s\n", ioc->name,
				__func__));
	if (ioc->open_pcie_trace)
		leapioraid_base_trace_log_exit(ioc);
	leapioraid_base_stop_watchdog(ioc);
	leapioraid_base_stop_hba_unplug_watchdog(ioc);
	leapioraid_base_free_resources(ioc);
	leapioraid_base_release_memory_pools(ioc);
	leapioraid_free_enclosure_list(ioc);
	pci_set_drvdata(ioc->pdev, NULL);
	kfree(ioc->cpu_msix_table);
	kfree(ioc->pd_handles);
	kfree(ioc->blocking_handles);
	kfree(ioc->tm_tr_retry);
	kfree(ioc->device_remove_in_progress);
	kfree(ioc->pend_os_device_add);
	kfree(ioc->pfacts);
	kfree(ioc->ctl_cmds.reply);
	kfree(ioc->ctl_cmds.sense);
	kfree(ioc->base_cmds.reply);
	kfree(ioc->port_enable_cmds.reply);
	kfree(ioc->tm_cmds.reply);
	kfree(ioc->transport_cmds.reply);
	kfree(ioc->scsih_cmds.reply);
	kfree(ioc->config_cmds.reply);
}

static void
leapioraid_base_clear_outstanding_leapioraid_commands(struct LEAPIORAID_ADAPTER
						   *ioc)
{
	struct leapioraid_internal_qcmd *scsih_qcmd, *scsih_qcmd_next;
	unsigned long flags;

	if (ioc->transport_cmds.status & LEAPIORAID_CMD_PENDING) {
		ioc->transport_cmds.status |= LEAPIORAID_CMD_RESET;
		leapioraid_base_free_smid(ioc, ioc->transport_cmds.smid);
		complete(&ioc->transport_cmds.done);
	}
	if (ioc->base_cmds.status & LEAPIORAID_CMD_PENDING) {
		ioc->base_cmds.status |= LEAPIORAID_CMD_RESET;
		leapioraid_base_free_smid(ioc, ioc->base_cmds.smid);
		complete(&ioc->base_cmds.done);
	}
	if (ioc->port_enable_cmds.status & LEAPIORAID_CMD_PENDING) {
		ioc->port_enable_failed = 1;
		ioc->port_enable_cmds.status |= LEAPIORAID_CMD_RESET;
		leapioraid_base_free_smid(ioc, ioc->port_enable_cmds.smid);
		if (ioc->is_driver_loading) {
			ioc->start_scan_failed =
			    LEAPIORAID_IOCSTATUS_INTERNAL_ERROR;
			ioc->start_scan = 0;
		} else
			complete(&ioc->port_enable_cmds.done);
	}
	if (ioc->config_cmds.status & LEAPIORAID_CMD_PENDING) {
		ioc->config_cmds.status |= LEAPIORAID_CMD_RESET;
		leapioraid_base_free_smid(ioc, ioc->config_cmds.smid);
		ioc->config_cmds.smid = USHORT_MAX;
		complete(&ioc->config_cmds.done);
	}
	spin_lock_irqsave(&ioc->scsih_q_internal_lock, flags);
	list_for_each_entry_safe(scsih_qcmd, scsih_qcmd_next,
				 &ioc->scsih_q_intenal_cmds, list) {
		if ((scsih_qcmd->status) & LEAPIORAID_CMD_PENDING) {
			scsih_qcmd->status |= LEAPIORAID_CMD_RESET;
			leapioraid_base_free_smid(ioc, scsih_qcmd->smid);
		}
	}
	spin_unlock_irqrestore(&ioc->scsih_q_internal_lock, flags);
}

static void
leapioraid_base_reset_handler(struct LEAPIORAID_ADAPTER *ioc, int reset_phase)
{
	leapioraid_scsihost_reset_handler(ioc, reset_phase);
	leapioraid_ctl_reset_handler(ioc, reset_phase);
	switch (reset_phase) {
	case LEAPIORAID_IOC_PRE_RESET_PHASE:
		dtmprintk(ioc, pr_info("%s %s: LEAPIORAID_IOC_PRE_RESET_PHASE\n",
			ioc->name, __func__));
		break;
	case LEAPIORAID_IOC_AFTER_RESET_PHASE:
		dtmprintk(ioc, pr_info("%s %s: LEAPIORAID_IOC_AFTER_RESET_PHASE\n",
			ioc->name, __func__));
		leapioraid_base_clear_outstanding_leapioraid_commands(ioc);
		break;
	case LEAPIORAID_IOC_DONE_RESET_PHASE:
		dtmprintk(ioc, pr_info("%s %s: LEAPIORAID_IOC_DONE_RESET_PHASE\n",
			ioc->name, __func__));
		break;
	}
}

void
leapioraid_wait_for_commands_to_complete(struct LEAPIORAID_ADAPTER *ioc)
{
	u32 ioc_state;
	unsigned long flags;
	u16 i;
	struct leapioraid_scsiio_tracker *st;

	ioc->pending_io_count = 0;
	if (!leapioraid_base_pci_device_is_available(ioc)) {
		pr_err("%s %s: pci error recovery reset or pci device unplug occurred\n",
			ioc->name, __func__);
		return;
	}
	ioc_state = leapioraid_base_get_iocstate(ioc, 0);
	if ((ioc_state & LEAPIORAID_IOC_STATE_MASK) !=
	    LEAPIORAID_IOC_STATE_OPERATIONAL)
		return;
	spin_lock_irqsave(&ioc->scsi_lookup_lock, flags);
	for (i = 1; i <= ioc->scsiio_depth; i++) {
		st = leapioraid_get_st_from_smid(ioc, i);
		if (st && st->smid != 0) {
			if (st->cb_idx != 0xFF)
				ioc->pending_io_count++;
		}
	}
	spin_unlock_irqrestore(&ioc->scsi_lookup_lock, flags);
	if (!ioc->pending_io_count)
		return;
	wait_event_timeout(ioc->reset_wq, ioc->pending_io_count == 0, 10 * HZ);
}

static int
leapioraid_base_check_ioc_facts_changes(struct LEAPIORAID_ADAPTER *ioc)
{
	u16 pd_handles_sz, tm_tr_retry_sz;
	void *pd_handles = NULL, *blocking_handles = NULL;
	void *pend_os_device_add = NULL, *device_remove_in_progress = NULL;
	u8 *tm_tr_retry = NULL;
	struct leapioraid_facts *old_facts = &ioc->prev_fw_facts;

	if (ioc->facts.MaxDevHandle > old_facts->MaxDevHandle) {
		pd_handles_sz = (ioc->facts.MaxDevHandle / 8);
		if (ioc->facts.MaxDevHandle % 8)
			pd_handles_sz++;
		pd_handles = krealloc(ioc->pd_handles, pd_handles_sz,
				      GFP_KERNEL);
		if (!pd_handles) {
			pr_err(
				"%s Unable to allocate the memory for pd_handles of sz: %d\n",
			    ioc->name, pd_handles_sz);
			return -ENOMEM;
		}
		memset(pd_handles + ioc->pd_handles_sz, 0,
		       (pd_handles_sz - ioc->pd_handles_sz));
		ioc->pd_handles = pd_handles;
		blocking_handles =
		    krealloc(ioc->blocking_handles, pd_handles_sz, GFP_KERNEL);
		if (!blocking_handles) {
			pr_err(
				"%s Unable to allocate the memory for blocking_handles of sz: %d\n",
			    ioc->name, pd_handles_sz);
			return -ENOMEM;
		}
		memset(blocking_handles + ioc->pd_handles_sz, 0,
		       (pd_handles_sz - ioc->pd_handles_sz));
		ioc->blocking_handles = blocking_handles;
		ioc->pd_handles_sz = pd_handles_sz;
		pend_os_device_add =
		    krealloc(ioc->pend_os_device_add, pd_handles_sz,
			     GFP_KERNEL);
		if (!pend_os_device_add) {
			pr_err(
				"%s Unable to allocate the memory for pend_os_device_add of sz: %d\n",
			    ioc->name, pd_handles_sz);
			return -ENOMEM;
		}
		memset(pend_os_device_add + ioc->pend_os_device_add_sz, 0,
		       (pd_handles_sz - ioc->pend_os_device_add_sz));
		ioc->pend_os_device_add = pend_os_device_add;
		ioc->pend_os_device_add_sz = pd_handles_sz;
		device_remove_in_progress =
		    krealloc(ioc->device_remove_in_progress, pd_handles_sz,
			     GFP_KERNEL);
		if (!device_remove_in_progress) {
			pr_err(
				"%s Unable to allocate the memory for device_remove_in_progress of sz: %d\n",
			    ioc->name, pd_handles_sz);
			return -ENOMEM;
		}
		memset(device_remove_in_progress +
		       ioc->device_remove_in_progress_sz, 0,
		       (pd_handles_sz - ioc->device_remove_in_progress_sz));
		ioc->device_remove_in_progress = device_remove_in_progress;
		ioc->device_remove_in_progress_sz = pd_handles_sz;
		tm_tr_retry_sz = ioc->facts.MaxDevHandle * sizeof(u8);
		tm_tr_retry = krealloc(ioc->tm_tr_retry, tm_tr_retry_sz,
				       GFP_KERNEL);
		if (!tm_tr_retry) {
			pr_err(
				"%s Unable to allocate the memory for tm_tr_retry of sz: %d\n",
			    ioc->name, tm_tr_retry_sz);
			return -ENOMEM;
		}
		memset(tm_tr_retry + ioc->tm_tr_retry_sz, 0,
		       (tm_tr_retry_sz - ioc->tm_tr_retry_sz));
		ioc->tm_tr_retry = tm_tr_retry;
		ioc->tm_tr_retry_sz = tm_tr_retry_sz;
	}
	memcpy(&ioc->prev_fw_facts, &ioc->facts,
	       sizeof(struct leapioraid_facts));
	return 0;
}

int
leapioraid_base_hard_reset_handler(
	struct LEAPIORAID_ADAPTER *ioc,
	enum reset_type type)
{
	int r;
	unsigned long flags;

	dtmprintk(ioc, pr_info("%s %s: enter\n", ioc->name,
			      __func__));
	if (!mutex_trylock(&ioc->reset_in_progress_mutex)) {
		do {
			ssleep(1);
		} while (ioc->shost_recovery == 1);
		dtmprintk(ioc,
			  pr_info("%s %s: exit\n", ioc->name,
				 __func__));
		return ioc->ioc_reset_status;
	}
	if (!leapioraid_base_pci_device_is_available(ioc)) {
		pr_err(
			"%s %s: pci error recovery reset or pci device unplug occurred\n",
			ioc->name, __func__);
		if (leapioraid_base_pci_device_is_unplugged(ioc)) {
			leapioraid_base_pause_mq_polling(ioc);
			ioc->schedule_dead_ioc_flush_running_cmds(ioc);
			leapioraid_base_resume_mq_polling(ioc);
		}
		r = 0;
		goto out_unlocked;
	}
	leapioraid_halt_firmware(ioc, 0);
	spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock, flags);
	ioc->shost_recovery = 1;
	spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
	leapioraid_base_get_iocstate(ioc, 0);
	leapioraid_base_reset_handler(ioc, LEAPIORAID_IOC_PRE_RESET_PHASE);
	leapioraid_wait_for_commands_to_complete(ioc);
	leapioraid_base_mask_interrupts(ioc);
	leapioraid_base_pause_mq_polling(ioc);
	r = leapioraid_base_make_ioc_ready(ioc, type);
	if (r)
		goto out;
	leapioraid_base_reset_handler(ioc, LEAPIORAID_IOC_AFTER_RESET_PHASE);
	if (ioc->is_driver_loading && ioc->port_enable_failed) {
		ioc->remove_host = 1;
		r = -EFAULT;
		goto out;
	}
	r = leapioraid_base_get_ioc_facts(ioc);
	if (r)
		goto out;
	r = leapioraid_base_check_ioc_facts_changes(ioc);
	if (r) {
		pr_err(
			"%s Some of the parameters got changed in this\n\t\t"
			"new firmware image and it requires system reboot\n",
				ioc->name);
		goto out;
	}
	if (ioc->rdpq_array_enable && !ioc->rdpq_array_capable)
		panic(
			"%s: Issue occurred with flashing controller firmware.\n\t\t"
			"Please reboot the system and ensure that the correct\n\t\t"
			"firmware version is running\n",
				ioc->name);
	r = leapioraid_base_make_ioc_operational(ioc);
	if (!r)
		leapioraid_base_reset_handler(ioc, LEAPIORAID_IOC_DONE_RESET_PHASE);
out:
	pr_info("%s %s: %s\n",
	       ioc->name, __func__, ((r == 0) ? "SUCCESS" : "FAILED"));
	spin_lock_irqsave(&ioc->ioc_reset_in_progress_lock, flags);
	ioc->ioc_reset_status = r;
	ioc->shost_recovery = 0;
	spin_unlock_irqrestore(&ioc->ioc_reset_in_progress_lock, flags);
	ioc->ioc_reset_count++;
	mutex_unlock(&ioc->reset_in_progress_mutex);
#if defined(DISABLE_RESET_SUPPORT)
	if (r != 0) {
		struct task_struct *p;

		ioc->remove_host = 1;
		ioc->schedule_dead_ioc_flush_running_cmds(ioc);
		p = kthread_run(leapioraid_remove_dead_ioc_func, ioc,
				"leapioraid_dead_ioc_%d", ioc->id);
		if (IS_ERR(p))
			pr_err(
				"%s %s: Running leapioraid_dead_ioc thread failed !!!!\n",
			    ioc->name, __func__);
		else
			pr_err(
				"%s %s: Running leapioraid_dead_ioc thread success !!!!\n",
			    ioc->name, __func__);
	}
#else
	if (r != 0)
		ioc->schedule_dead_ioc_flush_running_cmds(ioc);
#endif
	leapioraid_base_resume_mq_polling(ioc);
out_unlocked:
	dtmprintk(ioc, pr_info("%s %s: exit\n", ioc->name,
			      __func__));
	return r;
}

struct config_request {
	u16 sz;
	void *page;
	dma_addr_t page_dma;
};

static void
leapioraid_config_display_some_debug(struct LEAPIORAID_ADAPTER *ioc, u16 smid,
			   char *calling_function_name,
			   struct LeapioraidDefaultRep_t *mpi_reply)
{
	struct LeapioraidCfgReq_t *mpi_request;
	char *desc = NULL;

	mpi_request = leapioraid_base_get_msg_frame(ioc, smid);
	switch (mpi_request->Header.PageType & LEAPIORAID_CONFIG_PAGETYPE_MASK) {
	case LEAPIORAID_CONFIG_PAGETYPE_IO_UNIT:
		desc = "io_unit";
		break;
	case LEAPIORAID_CONFIG_PAGETYPE_IOC:
		desc = "ioc";
		break;
	case LEAPIORAID_CONFIG_PAGETYPE_BIOS:
		desc = "bios";
		break;
	case LEAPIORAID_CONFIG_PAGETYPE_RAID_VOLUME:
		desc = "raid_volume";
		break;
	case LEAPIORAID_CONFIG_PAGETYPE_MANUFACTURING:
		desc = "manufacturing";
		break;
	case LEAPIORAID_CONFIG_PAGETYPE_RAID_PHYSDISK:
		desc = "physdisk";
		break;
	case LEAPIORAID_CONFIG_PAGETYPE_EXTENDED:
		switch (mpi_request->ExtPageType) {
		case LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_IO_UNIT:
			desc = "sas_io_unit";
			break;
		case LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_EXPANDER:
			desc = "sas_expander";
			break;
		case LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_DEVICE:
			desc = "sas_device";
			break;
		case LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_PHY:
			desc = "sas_phy";
			break;
		case LEAPIORAID_CONFIG_EXTPAGETYPE_LOG:
			desc = "log";
			break;
		case LEAPIORAID_CONFIG_EXTPAGETYPE_ENCLOSURE:
			desc = "enclosure";
			break;
		case LEAPIORAID_CONFIG_EXTPAGETYPE_RAID_CONFIG:
			desc = "raid_config";
			break;
		case LEAPIORAID_CONFIG_EXTPAGETYPE_DRIVER_MAPPING:
			desc = "driver_mapping";
			break;
		case LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_PORT:
			desc = "sas_port";
			break;
		case LEAPIORAID_CONFIG_EXTPAGETYPE_EXT_MANUFACTURING:
			desc = "ext_manufacturing";
			break;
		}
		break;
	}
	if (!desc)
		return;
	pr_info("%s %s: %s(%d), action(%d), form(0x%08x), smid(%d)\n",
		ioc->name, calling_function_name, desc,
		mpi_request->Header.PageNumber, mpi_request->Action,
		le32_to_cpu(mpi_request->PageAddress), smid);
	if (!mpi_reply)
		return;
	if (mpi_reply->IOCStatus || mpi_reply->IOCLogInfo)
		pr_err(
		       "%s \tiocstatus(0x%04x), loginfo(0x%08x)\n",
		       ioc->name, le16_to_cpu(mpi_reply->IOCStatus),
		       le32_to_cpu(mpi_reply->IOCLogInfo));
}

static int
leapioraid_config_alloc_config_dma_memory(struct LEAPIORAID_ADAPTER *ioc,
				struct config_request *mem)
{
	int r = 0;

	if (mem->sz > ioc->config_page_sz) {
		mem->page = dma_alloc_coherent(&ioc->pdev->dev, mem->sz,
					       &mem->page_dma, GFP_KERNEL);
		if (!mem->page)
			r = -ENOMEM;
	} else {
		mem->page = ioc->config_page;
		mem->page_dma = ioc->config_page_dma;
	}
	ioc->config_vaddr = mem->page;
	return r;
}

static void
leapioraid_config_free_config_dma_memory(struct LEAPIORAID_ADAPTER *ioc,
			       struct config_request *mem)
{
	if (mem->sz > ioc->config_page_sz)
		dma_free_coherent(&ioc->pdev->dev, mem->sz, mem->page,
				  mem->page_dma);
}

u8
leapioraid_config_done(
	struct LEAPIORAID_ADAPTER *ioc, u16 smid, u8 msix_index,
	u32 reply)
{
	struct LeapioraidDefaultRep_t *mpi_reply;

	if (ioc->config_cmds.status == LEAPIORAID_CMD_NOT_USED)
		return 1;
	if (ioc->config_cmds.smid != smid)
		return 1;
	ioc->config_cmds.status |= LEAPIORAID_CMD_COMPLETE;
	mpi_reply = leapioraid_base_get_reply_virt_addr(ioc, reply);
	if (mpi_reply) {
		ioc->config_cmds.status |= LEAPIORAID_CMD_REPLY_VALID;
		memcpy(ioc->config_cmds.reply, mpi_reply,
		       mpi_reply->MsgLength * 4);
	}
	ioc->config_cmds.status &= ~LEAPIORAID_CMD_PENDING;
	if (ioc->logging_level & LEAPIORAID_DEBUG_CONFIG)
		leapioraid_config_display_some_debug(
			ioc, smid, "config_done", mpi_reply);
	ioc->config_cmds.smid = USHORT_MAX;
	complete(&ioc->config_cmds.done);
	return 1;
}

static int
leapioraid_config_request(
	struct LEAPIORAID_ADAPTER *ioc, struct LeapioraidCfgReq_t *mpi_request,
	struct LeapioraidCfgRep_t *mpi_reply, int timeout,
	void *config_page, u16 config_page_sz)
{
	u16 smid;
	struct LeapioraidCfgReq_t *config_request;
	int r;
	u8 retry_count, issue_host_reset = 0;
	struct config_request mem;
	u32 ioc_status = UINT_MAX;
	u8 issue_reset;

	mutex_lock(&ioc->config_cmds.mutex);
	if (ioc->config_cmds.status != LEAPIORAID_CMD_NOT_USED) {
		pr_err("%s %s: config_cmd in use\n",
		       ioc->name, __func__);
		mutex_unlock(&ioc->config_cmds.mutex);
		return -EAGAIN;
	}
	retry_count = 0;
	memset(&mem, 0, sizeof(struct config_request));
	mpi_request->VF_ID = 0;
	mpi_request->VP_ID = 0;
	if (config_page) {
		mpi_request->Header.PageVersion = mpi_reply->Header.PageVersion;
		mpi_request->Header.PageNumber = mpi_reply->Header.PageNumber;
		mpi_request->Header.PageType = mpi_reply->Header.PageType;
		mpi_request->Header.PageLength = mpi_reply->Header.PageLength;
		mpi_request->ExtPageLength = mpi_reply->ExtPageLength;
		mpi_request->ExtPageType = mpi_reply->ExtPageType;
		if (mpi_request->Header.PageLength)
			mem.sz = mpi_request->Header.PageLength * 4;
		else
			mem.sz = le16_to_cpu(mpi_reply->ExtPageLength) * 4;
		r = leapioraid_config_alloc_config_dma_memory(ioc, &mem);
		if (r != 0)
			goto out;
		if (mpi_request->Action ==
		    LEAPIORAID_CONFIG_ACTION_PAGE_WRITE_CURRENT ||
		    mpi_request->Action ==
		    LEAPIORAID_CONFIG_ACTION_PAGE_WRITE_NVRAM) {
			ioc->base_add_sg_single(&mpi_request->PageBufferSGE,
						LEAPIORAID_CONFIG_COMMON_WRITE_SGLFLAGS
						| mem.sz, mem.page_dma);
			memcpy(mem.page, config_page,
			       min_t(u16, mem.sz, config_page_sz));
		} else {
			memset(config_page, 0, config_page_sz);
			ioc->base_add_sg_single(&mpi_request->PageBufferSGE,
						LEAPIORAID_CONFIG_COMMON_SGLFLAGS
						| mem.sz, mem.page_dma);
			memset(mem.page, 0, min_t(u16, mem.sz, config_page_sz));
		}
	}
retry_config:
	if (retry_count) {
		if (retry_count > 2) {
			r = -EFAULT;
			goto free_mem;
		}
		pr_info("%s %s: attempting retry (%d)\n",
		       ioc->name, __func__, retry_count);
	}
	r = leapioraid_wait_for_ioc_to_operational(ioc,
						   LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT);
	if (r) {
		if (r == -ETIME)
			issue_host_reset = 1;
		goto free_mem;
	}
	smid = leapioraid_base_get_smid(ioc, ioc->config_cb_idx);
	if (!smid) {
		pr_err("%s %s: failed obtaining a smid\n",
		       ioc->name, __func__);
		ioc->config_cmds.status = LEAPIORAID_CMD_NOT_USED;
		r = -EAGAIN;
		goto free_mem;
	}
	r = 0;
	memset(mpi_reply, 0, sizeof(struct LeapioraidCfgRep_t));
	memset(ioc->config_cmds.reply, 0, sizeof(struct LeapioraidCfgRep_t));
	ioc->config_cmds.status = LEAPIORAID_CMD_PENDING;
	config_request = leapioraid_base_get_msg_frame(ioc, smid);
	ioc->config_cmds.smid = smid;
	memcpy(config_request, mpi_request, sizeof(struct LeapioraidCfgReq_t));
	if (ioc->logging_level & LEAPIORAID_DEBUG_CONFIG)
		leapioraid_config_display_some_debug(ioc, smid, "config_request", NULL);
	init_completion(&ioc->config_cmds.done);
	ioc->put_smid_default(ioc, smid);
	wait_for_completion_timeout(&ioc->config_cmds.done, timeout * HZ);
	if (!(ioc->config_cmds.status & LEAPIORAID_CMD_COMPLETE)) {
		if (!(ioc->logging_level & LEAPIORAID_DEBUG_CONFIG))
			leapioraid_config_display_some_debug(ioc, smid,
						   "config_request no reply",
						   NULL);
		leapioraid_check_cmd_timeout(ioc, ioc->config_cmds.status,
					     mpi_request,
					     sizeof(struct LeapioraidCfgReq_t) / 4,
					     issue_reset);
		pr_info("%s issue_reset=%d\n", __func__, issue_reset);
		retry_count++;
		if (ioc->config_cmds.smid == smid)
			leapioraid_base_free_smid(ioc, smid);
		if (ioc->config_cmds.status & LEAPIORAID_CMD_RESET)
			goto retry_config;
		if (ioc->shost_recovery || ioc->pci_error_recovery) {
			issue_host_reset = 0;
			r = -EFAULT;
		} else
			issue_host_reset = 1;
		goto free_mem;
	}
	if (ioc->config_cmds.status & LEAPIORAID_CMD_REPLY_VALID) {
		memcpy(mpi_reply, ioc->config_cmds.reply,
		       sizeof(struct LeapioraidCfgRep_t));
		if ((mpi_request->Header.PageType & 0xF) !=
		    (mpi_reply->Header.PageType & 0xF)) {
			if (!(ioc->logging_level & LEAPIORAID_DEBUG_CONFIG))
				leapioraid_config_display_some_debug(ioc, smid,
							   "config_request",
							   NULL);
			leapioraid_debug_dump_mf(mpi_request, ioc->request_sz / 4);
			leapioraid_debug_dump_reply(mpi_reply, ioc->reply_sz / 4);
			panic(
				"%s %s: Firmware BUG: mpi_reply mismatch:\n\t\t"
				"Requested PageType(0x%02x) Reply PageType(0x%02x)\n",
					ioc->name,
					__func__,
					(mpi_request->Header.PageType & 0xF),
					(mpi_reply->Header.PageType & 0xF));
		}
		if (((mpi_request->Header.PageType & 0xF) ==
		     LEAPIORAID_CONFIG_PAGETYPE_EXTENDED) &&
		    mpi_request->ExtPageType != mpi_reply->ExtPageType) {
			if (!(ioc->logging_level & LEAPIORAID_DEBUG_CONFIG))
				leapioraid_config_display_some_debug(ioc, smid,
							   "config_request",
							   NULL);
			leapioraid_debug_dump_mf(mpi_request, ioc->request_sz / 4);
			leapioraid_debug_dump_reply(mpi_reply, ioc->reply_sz / 4);
			panic(
				"%s %s: Firmware BUG: mpi_reply mismatch:\n\t\t"
				"Requested ExtPageType(0x%02x) Reply ExtPageType(0x%02x)\n",
					ioc->name,
					__func__,
					mpi_request->ExtPageType,
					mpi_reply->ExtPageType);
		}
		ioc_status = le16_to_cpu(mpi_reply->IOCStatus)
		    & LEAPIORAID_IOCSTATUS_MASK;
	}
	if (retry_count)
		pr_info("%s %s: retry (%d) completed!!\n",
		       ioc->name, __func__, retry_count);
	if ((ioc_status == LEAPIORAID_IOCSTATUS_SUCCESS) &&
	    config_page && mpi_request->Action ==
	    LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT) {
		u8 *p = (u8 *) mem.page;

		if (p) {
			if ((mpi_request->Header.PageType & 0xF) !=
			    (p[3] & 0xF)) {
				if (!
				    (ioc->logging_level & LEAPIORAID_DEBUG_CONFIG))
					leapioraid_config_display_some_debug(ioc, smid,
								   "config_request",
								   NULL);
				leapioraid_debug_dump_mf(mpi_request,
					       ioc->request_sz / 4);
				leapioraid_debug_dump_reply(mpi_reply, ioc->reply_sz / 4);
				leapioraid_debug_dump_config(p, min_t(u16, mem.sz,
							    config_page_sz) /
						   4);
				panic(
					"%s %s: Firmware BUG: config page mismatch:\n\t\t"
					"Requested PageType(0x%02x) Reply PageType(0x%02x)\n",
						ioc->name,
						__func__,
						(mpi_request->Header.PageType & 0xF),
						(p[3] & 0xF));
			}
			if (((mpi_request->Header.PageType & 0xF) ==
			     LEAPIORAID_CONFIG_PAGETYPE_EXTENDED) &&
			    (mpi_request->ExtPageType != p[6])) {
				if (!
				    (ioc->logging_level & LEAPIORAID_DEBUG_CONFIG))
					leapioraid_config_display_some_debug(ioc, smid,
								   "config_request",
								   NULL);
				leapioraid_debug_dump_mf(mpi_request,
					       ioc->request_sz / 4);
				leapioraid_debug_dump_reply(mpi_reply, ioc->reply_sz / 4);
				leapioraid_debug_dump_config(p, min_t(u16, mem.sz,
							    config_page_sz) /
						   4);
				panic(
					"%s %s: Firmware BUG: config page mismatch:\n\t\t"
					"Requested ExtPageType(0x%02x) Reply ExtPageType(0x%02x)\n",
						ioc->name,
						__func__,
						mpi_request->ExtPageType,
						p[6]);
			}
		}
		memcpy(config_page, mem.page, min_t(u16, mem.sz,
						    config_page_sz));
	}
free_mem:
	if (config_page)
		leapioraid_config_free_config_dma_memory(ioc, &mem);
out:
	ioc->config_cmds.status = LEAPIORAID_CMD_NOT_USED;
	mutex_unlock(&ioc->config_cmds.mutex);
	if (issue_host_reset) {
		if (ioc->drv_internal_flags & LEAPIORAID_DRV_INERNAL_FIRST_PE_ISSUED) {
			leapioraid_base_hard_reset_handler(ioc,
							   FORCE_BIG_HAMMER);
			r = -EFAULT;
		} else {
			if (leapioraid_base_check_for_fault_and_issue_reset
			    (ioc))
				return -EFAULT;
			r = -EAGAIN;
		}
	}
	return r;
}

int
leapioraid_config_get_manufacturing_pg0(struct LEAPIORAID_ADAPTER *ioc,
					struct LeapioraidCfgRep_t *mpi_reply,
					struct LeapioraidManP0_t *
					config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_MANUFACTURING;
	mpi_request.Header.PageNumber = 0;
	mpi_request.Header.PageVersion = 0x00;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_manufacturing_pg10(struct LEAPIORAID_ADAPTER *ioc,
					 struct LeapioraidCfgRep_t *mpi_reply,
					 struct LeapioraidManuP10_t *config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_MANUFACTURING;
	mpi_request.Header.PageNumber = 10;
	mpi_request.Header.PageVersion = 0x00;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_manufacturing_pg11(struct LEAPIORAID_ADAPTER *ioc,
					 struct LeapioraidCfgRep_t *mpi_reply,
					 struct LeapioraidManuP11_t
					 *config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_MANUFACTURING;
	mpi_request.Header.PageNumber = 11;
	mpi_request.Header.PageVersion = 0x00;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_set_manufacturing_pg11(struct LEAPIORAID_ADAPTER *ioc,
					 struct LeapioraidCfgRep_t *mpi_reply,
					 struct LeapioraidManuP11_t
					 *config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_MANUFACTURING;
	mpi_request.Header.PageNumber = 11;
	mpi_request.Header.PageVersion = 0x00;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_WRITE_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_bios_pg2(struct LEAPIORAID_ADAPTER *ioc,
			       struct LeapioraidCfgRep_t *mpi_reply,
			       struct LeapioraidBiosP2_t *config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_BIOS;
	mpi_request.Header.PageNumber = 2;
	mpi_request.Header.PageVersion = 0x04;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_bios_pg3(struct LEAPIORAID_ADAPTER *ioc,
			       struct LeapioraidCfgRep_t *mpi_reply,
			       struct LeapioraidBiosP3_t *config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_BIOS;
	mpi_request.Header.PageNumber = 3;
	mpi_request.Header.PageVersion = 0x01;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_iounit_pg0(struct LEAPIORAID_ADAPTER *ioc,
				 struct LeapioraidCfgRep_t *mpi_reply,
				 struct LeapioraidIOUnitP0_t *config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_IO_UNIT;
	mpi_request.Header.PageNumber = 0;
	mpi_request.Header.PageVersion = 0x02;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_iounit_pg1(struct LEAPIORAID_ADAPTER *ioc,
				 struct LeapioraidCfgRep_t *mpi_reply,
				 struct LeapioraidIOUnitP1_t *config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_IO_UNIT;
	mpi_request.Header.PageNumber = 1;
	mpi_request.Header.PageVersion = 0x04;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_set_iounit_pg1(struct LEAPIORAID_ADAPTER *ioc,
				 struct LeapioraidCfgRep_t *mpi_reply,
				 struct LeapioraidIOUnitP1_t *config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_IO_UNIT;
	mpi_request.Header.PageNumber = 1;
	mpi_request.Header.PageVersion = 0x04;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_WRITE_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_iounit_pg8(struct LEAPIORAID_ADAPTER *ioc,
				 struct LeapioraidCfgRep_t *mpi_reply,
				 struct LeapioraidIOUnitP8_t *config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_IO_UNIT;
	mpi_request.Header.PageNumber = 8;
	mpi_request.Header.PageVersion = 0x00;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_ioc_pg1(struct LEAPIORAID_ADAPTER *ioc,
			      struct LeapioraidCfgRep_t *mpi_reply,
			      struct LeapioraidIOCP1_t *config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_IOC;
	mpi_request.Header.PageNumber = 1;
	mpi_request.Header.PageVersion = 0x00;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_set_ioc_pg1(struct LEAPIORAID_ADAPTER *ioc,
			      struct LeapioraidCfgRep_t *mpi_reply,
			      struct LeapioraidIOCP1_t *config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_IOC;
	mpi_request.Header.PageNumber = 1;
	mpi_request.Header.PageVersion = 0x00;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_WRITE_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_ioc_pg8(struct LEAPIORAID_ADAPTER *ioc,
			      struct LeapioraidCfgRep_t *mpi_reply,
			      struct LeapioraidIOCP8_t *config_page)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_IOC;
	mpi_request.Header.PageNumber = 8;
	mpi_request.Header.PageVersion = 0x00;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_sas_device_pg0(struct LEAPIORAID_ADAPTER *ioc,
				     struct LeapioraidCfgRep_t *mpi_reply,
				     struct LeapioraidSasDevP0_t *config_page,
				     u32 form, u32 handle)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_EXTENDED;
	mpi_request.ExtPageType = LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_DEVICE;
	mpi_request.Header.PageVersion = 0x09;
	mpi_request.Header.PageNumber = 0;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.PageAddress = cpu_to_le32(form | handle);
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_number_hba_phys(struct LEAPIORAID_ADAPTER *ioc,
				      u8 *num_phys)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;
	u16 ioc_status;
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidSasIOUnitP0_t config_page;

	*num_phys = 0;
	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_EXTENDED;
	mpi_request.ExtPageType = LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_IO_UNIT;
	mpi_request.Header.PageNumber = 0;
	mpi_request.Header.PageVersion = 0x05;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, &mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, &mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, &config_page,
			    sizeof(struct LeapioraidSasIOUnitP0_t));
	if (!r) {
		ioc_status = le16_to_cpu(mpi_reply.IOCStatus) &
		    LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status == LEAPIORAID_IOCSTATUS_SUCCESS)
			*num_phys = config_page.NumPhys;
	}
out:
	return r;
}

int
leapioraid_config_get_sas_iounit_pg0(struct LEAPIORAID_ADAPTER *ioc,
				     struct LeapioraidCfgRep_t *mpi_reply,
				     struct LeapioraidSasIOUnitP0_t *config_page,
				     u16 sz)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_EXTENDED;
	mpi_request.ExtPageType = LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_IO_UNIT;
	mpi_request.Header.PageNumber = 0;
	mpi_request.Header.PageVersion = 0x05;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sz);
out:
	return r;
}

int
leapioraid_config_get_sas_iounit_pg1(struct LEAPIORAID_ADAPTER *ioc,
				     struct LeapioraidCfgRep_t *mpi_reply,
				     struct LeapioraidSasIOUnitP1_t *config_page,
				     u16 sz)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_EXTENDED;
	mpi_request.ExtPageType = LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_IO_UNIT;
	mpi_request.Header.PageNumber = 1;
	mpi_request.Header.PageVersion = 0x09;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sz);
out:
	return r;
}

int
leapioraid_config_set_sas_iounit_pg1(struct LEAPIORAID_ADAPTER *ioc,
				     struct LeapioraidCfgRep_t *mpi_reply,
				     struct LeapioraidSasIOUnitP1_t *config_page,
				     u16 sz)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_EXTENDED;
	mpi_request.ExtPageType = LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_IO_UNIT;
	mpi_request.Header.PageNumber = 1;
	mpi_request.Header.PageVersion = 0x09;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_WRITE_CURRENT;
	leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page, sz);
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_WRITE_NVRAM;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sz);
out:
	return r;
}

int
leapioraid_config_get_expander_pg0(struct LEAPIORAID_ADAPTER *ioc,
				   struct LeapioraidCfgRep_t *mpi_reply,
				   struct LeapioraidExpanderP0_t *config_page,
				   u32 form, u32 handle)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_EXTENDED;
	mpi_request.ExtPageType = LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_EXPANDER;
	mpi_request.Header.PageNumber = 0;
	mpi_request.Header.PageVersion = 0x06;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.PageAddress = cpu_to_le32(form | handle);
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_expander_pg1(struct LEAPIORAID_ADAPTER *ioc,
				   struct LeapioraidCfgRep_t *mpi_reply,
				   struct LeapioraidExpanderP1_t *config_page,
				   u32 phy_number, u16 handle)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_EXTENDED;
	mpi_request.ExtPageType = LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_EXPANDER;
	mpi_request.Header.PageNumber = 1;
	mpi_request.Header.PageVersion = 0x02;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.PageAddress =
	    cpu_to_le32(LEAPIORAID_SAS_EXPAND_PGAD_FORM_HNDL_PHY_NUM |
			(phy_number << LEAPIORAID_SAS_EXPAND_PGAD_PHYNUM_SHIFT) |
			handle);
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_enclosure_pg0(struct LEAPIORAID_ADAPTER *ioc,
				    struct LeapioraidCfgRep_t *mpi_reply,
				    struct LeapioraidSasEncP0_t *config_page,
				    u32 form, u32 handle)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_EXTENDED;
	mpi_request.ExtPageType = LEAPIORAID_CONFIG_EXTPAGETYPE_ENCLOSURE;
	mpi_request.Header.PageNumber = 0;
	mpi_request.Header.PageVersion = 0x04;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.PageAddress = cpu_to_le32(form | handle);
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_phy_pg0(struct LEAPIORAID_ADAPTER *ioc,
			      struct LeapioraidCfgRep_t *mpi_reply,
			      struct LeapioraidSasPhyP0_t *config_page,
			      u32 phy_number)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_EXTENDED;
	mpi_request.ExtPageType = LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_PHY;
	mpi_request.Header.PageNumber = 0;
	mpi_request.Header.PageVersion = 0x03;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.PageAddress =
	    cpu_to_le32(LEAPIORAID_SAS_PHY_PGAD_FORM_PHY_NUMBER | phy_number);
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_phy_pg1(struct LEAPIORAID_ADAPTER *ioc,
			      struct LeapioraidCfgRep_t *mpi_reply,
			      struct LeapioraidSasPhyP1_t *config_page,
			      u32 phy_number)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_EXTENDED;
	mpi_request.ExtPageType = LEAPIORAID_CONFIG_EXTPAGETYPE_SAS_PHY;
	mpi_request.Header.PageNumber = 1;
	mpi_request.Header.PageVersion = 0x01;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.PageAddress =
	    cpu_to_le32(LEAPIORAID_SAS_PHY_PGAD_FORM_PHY_NUMBER | phy_number);
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_raid_volume_pg1(struct LEAPIORAID_ADAPTER *ioc,
				      struct LeapioraidCfgRep_t *mpi_reply,
				      struct LeapioraidRaidVolP1_t *config_page,
				      u32 form, u32 handle)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_RAID_VOLUME;
	mpi_request.Header.PageNumber = 1;
	mpi_request.Header.PageVersion = 0x03;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.PageAddress = cpu_to_le32(form | handle);
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_number_pds(struct LEAPIORAID_ADAPTER *ioc,
	u16 handle, u8 *num_pds)
{
	struct LeapioraidCfgReq_t mpi_request;
	struct LeapioraidRaidVolP0_t config_page;
	struct LeapioraidCfgRep_t mpi_reply;
	int r;
	u16 ioc_status;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	*num_pds = 0;
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_RAID_VOLUME;
	mpi_request.Header.PageNumber = 0;
	mpi_request.Header.PageVersion = 0x0A;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, &mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.PageAddress =
	    cpu_to_le32(LEAPIORAID_RAID_VOLUME_PGAD_FORM_HANDLE | handle);
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, &mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, &config_page,
			    sizeof(struct LeapioraidRaidVolP0_t));
	if (!r) {
		ioc_status = le16_to_cpu(mpi_reply.IOCStatus) &
		    LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status == LEAPIORAID_IOCSTATUS_SUCCESS)
			*num_pds = config_page.NumPhysDisks;
	}
out:
	return r;
}

int
leapioraid_config_get_raid_volume_pg0(struct LEAPIORAID_ADAPTER *ioc,
				      struct LeapioraidCfgRep_t *mpi_reply,
				      struct LeapioraidRaidVolP0_t *config_page,
				      u32 form, u32 handle, u16 sz)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_RAID_VOLUME;
	mpi_request.Header.PageNumber = 0;
	mpi_request.Header.PageVersion = 0x0A;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.PageAddress = cpu_to_le32(form | handle);
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sz);
out:
	return r;
}

int
leapioraid_config_get_phys_disk_pg0(struct LEAPIORAID_ADAPTER *ioc,
				    struct LeapioraidCfgRep_t *mpi_reply,
				    struct LeapioraidRaidPDP0_t *config_page,
				    u32 form, u32 form_specific)
{
	struct LeapioraidCfgReq_t mpi_request;
	int r;

	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_RAID_PHYSDISK;
	mpi_request.Header.PageNumber = 0;
	mpi_request.Header.PageVersion = 0x05;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.PageAddress = cpu_to_le32(form | form_specific);
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	r = leapioraid_config_request(ioc, &mpi_request, mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, config_page,
			    sizeof(*config_page));
out:
	return r;
}

int
leapioraid_config_get_volume_handle(struct LEAPIORAID_ADAPTER *ioc,
				    u16 pd_handle, u16 *volume_handle)
{
	struct LeapioraidRaidCfgP0_t *config_page = NULL;
	struct LeapioraidCfgReq_t mpi_request;
	struct LeapioraidCfgRep_t mpi_reply;
	int r, i, config_page_sz;
	u16 ioc_status;
	int config_num;
	u16 element_type;
	u16 phys_disk_dev_handle;

	*volume_handle = 0;
	memset(&mpi_request, 0, sizeof(struct LeapioraidCfgReq_t));
	mpi_request.Function = LEAPIORAID_FUNC_CONFIG;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_HEADER;
	mpi_request.Header.PageType = LEAPIORAID_CONFIG_PAGETYPE_EXTENDED;
	mpi_request.ExtPageType = LEAPIORAID_CONFIG_EXTPAGETYPE_RAID_CONFIG;
	mpi_request.Header.PageVersion = 0x00;
	mpi_request.Header.PageNumber = 0;
	ioc->build_zero_len_sge_mpi(ioc, &mpi_request.PageBufferSGE);
	r = leapioraid_config_request(ioc, &mpi_request, &mpi_reply,
			    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT, NULL, 0);
	if (r)
		goto out;
	mpi_request.Action = LEAPIORAID_CONFIG_ACTION_PAGE_READ_CURRENT;
	config_page_sz = (le16_to_cpu(mpi_reply.ExtPageLength) * 4);
	config_page = kmalloc(config_page_sz, GFP_KERNEL);
	if (!config_page) {
		r = -1;
		goto out;
	}
	config_num = 0xff;
	while (1) {
		mpi_request.PageAddress = cpu_to_le32(config_num +
						      LEAPIORAID_RAID_PGAD_FORM_GET_NEXT_CONFIGNUM);
		r = leapioraid_config_request(ioc, &mpi_request, &mpi_reply,
				    LEAPIORAID_CONFIG_PAGE_DEFAULT_TIMEOUT,
				    config_page, config_page_sz);
		if (r)
			goto out;
		r = -1;
		ioc_status = le16_to_cpu(mpi_reply.IOCStatus) &
		    LEAPIORAID_IOCSTATUS_MASK;
		if (ioc_status == LEAPIORAID_IOCSTATUS_CONFIG_INVALID_PAGE) {
			*volume_handle = 0;
			r = 0;
			goto out;
		} else if (ioc_status != LEAPIORAID_IOCSTATUS_SUCCESS)
			goto out;
		for (i = 0; i < config_page->NumElements; i++) {
			element_type =
			    le16_to_cpu(config_page->ConfigElement[i].ElementFlags) &
			    LEAPIORAID_RAIDCONFIG0_EFLAGS_MASK_ELEMENT_TYPE;
			if (element_type ==
			    LEAPIORAID_RAIDCONFIG0_EFLAGS_VOL_PHYS_DISK_ELEMENT
			    || element_type ==
			    LEAPIORAID_RAIDCONFIG0_EFLAGS_OCE_ELEMENT) {
				phys_disk_dev_handle =
				    le16_to_cpu(config_page->ConfigElement[i].PhysDiskDevHandle);
				if (phys_disk_dev_handle == pd_handle) {
					*volume_handle =
					    le16_to_cpu
					    (config_page->ConfigElement[i].VolDevHandle);
					r = 0;
					goto out;
				}
			} else if (element_type ==
				   LEAPIORAID_RAIDCONFIG0_EFLAGS_HOT_SPARE_ELEMENT) {
				*volume_handle = 0;
				r = 0;
				goto out;
			}
		}
		config_num = config_page->ConfigNum;
	}
out:
	kfree(config_page);
	return r;
}

int
leapioraid_config_get_volume_wwid(struct LEAPIORAID_ADAPTER *ioc,
				  u16 volume_handle, u64 *wwid)
{
	struct LeapioraidCfgRep_t mpi_reply;
	struct LeapioraidRaidVolP1_t raid_vol_pg1;

	*wwid = 0;
	if (!(leapioraid_config_get_raid_volume_pg1(ioc, &mpi_reply,
						    &raid_vol_pg1,
						    LEAPIORAID_RAID_VOLUME_PGAD_FORM_HANDLE,
						    volume_handle))) {
		*wwid = le64_to_cpu(raid_vol_pg1.WWID);
		return 0;
	} else
		return -1;
}
