// SPDX-License-Identifier: GPL-2.0-only
/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
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
 */

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/errno.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include "mvx_if.h"
#include "mvx_hwreg.h"
#include "mvx_mmu.h"
#include "mvx_scheduler.h"
#include "mvx_session.h"
#include "mvx_seq.h"
#include "mvx_pm_runtime.h"
#include "mvx_log_group.h"

/****************************************************************************
 * Private variables
 ****************************************************************************/

static int wait_scheduler_timeout = 1000;
module_param(wait_scheduler_timeout, int, 0660);

/****************************************************************************
 * Static functions
 ****************************************************************************/

static struct mvx_lsid *find_free_lsid(struct mvx_sched *sched)
{
	unsigned int i;

	for (i = 0; i < sched->nlsid; i++)
		if (sched->lsid[i].session == NULL)
			return &sched->lsid[i];

	return NULL;
}

static struct mvx_lsid *find_idle_lsid(struct mvx_sched *sched)
{
	unsigned int i;

	for (i = 0; i < sched->nlsid; i++) {
		bool idle;

		idle = mvx_lsid_idle(&sched->lsid[i]);
		if (idle != false)
			return &sched->lsid[i];
	}

	return NULL;
}

static int map_session(struct mvx_sched *sched,
		       struct mvx_sched_session *session, struct mvx_lsid *lsid)
{
	int ret;

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
		      "%p Map LSID. lsid=%u, jobqueue=%08x, corelsid=%08x.",
		      mvx_if_session_to_session(session->isession),
		      lsid->lsid,
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_JOBQUEUE),
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_CORELSID));

	ret = mvx_lsid_map(lsid, &session->pcb);
	if (ret != 0)
		return ret;

	session->lsid = lsid;
	lsid->session = session;

	return 0;
}

static void unmap_session(struct mvx_sched *sched,
			  struct mvx_sched_session *session)
{
	struct mvx_lsid *lsid = session->lsid;

	if (lsid == NULL)
		return;

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
		      "%p Unmap LSID. lsid=%u, jobqueue=%08x, corelsid=%08x.",
		      mvx_if_session_to_session(session->isession),
		      lsid->lsid,
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_JOBQUEUE),
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_CORELSID));

	mvx_lsid_unmap(lsid, &session->pcb);
	session->lsid = NULL;
	lsid->session = NULL;
}

static struct list_head *list_find_node(struct list_head *list,
					struct list_head *node)
{
	struct list_head *i;

	list_for_each(i, list) {
		if (i == node)
			return i;
	}

	return NULL;
}

static void set_sched_state(struct mvx_sched *sched, int state)
{
	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
		      "switch scheduler state, %d -> %d", sched->state, state);
	sched->state = state;
}

static uint32_t sort_jobs(uint32_t jobqueue, struct mvx_lsid lsid[MVX_LSID_MAX])
{
	int i;
	uint32_t new_jobqueue = 0x0F0F0F0F;
	uint32_t new_job = 0, next_job = 0;
	uint32_t id, ncores;
	struct mvx_lsid *new = NULL, *next = NULL;

	/* Find the new job to be sorted */
	for (i = 24; i >= 0; i -= 8) {
		new_job = (jobqueue >> i) & 0xFF;
		if (new_job != MVE_JOBQUEUE_JOB_INVALID) {
			if (new_job == MVE_JOBQUEUE_JOB_OBSOLETED)
				MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
					      "Found obsoleted job in the end of jobqueue, remove it");
			else
				break;
		}
	}
	id = new_job & 0x0F;
	if (id < MVX_LSID_MAX) {
		new = &lsid[id];
	} else {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Invalid job lsid %d in jobqueue %08x", id,
			      jobqueue);
		return jobqueue;	// Invalid job, return original queue
	}

	/* Insert the new job into the job queue */
	for (i -= 8; i >= 0; i -= 8) {
		next_job = (jobqueue >> i) & 0xFF;
		id = next_job & 0x0F;
		if (id < MVX_LSID_MAX) {
			next = &lsid[id];
		} else {
			if (next_job == MVE_JOBQUEUE_JOB_OBSOLETED) {
				MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
					      "Found obsoleted job in jobqueue, remove it");
				continue;
			} else {
				MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
					      "Invalid job lsid %d in jobqueue %08x",
					      id, jobqueue);
				return jobqueue;	// Invalid job, return original queue
			}
		}

		ncores = (next_job >> 4) + 1;
		if ((new->session->priority >= next->session->priority_in_queue)
		    || (i == 0 && ncores < next->session->isession->ncores))
			break;

		new_jobqueue = (new_jobqueue << 8) | next_job;
		if (next->session->priority_in_queue > 1 &&
		    new->session->priority > 0)
			next->session->priority_in_queue--;
	}
	new_jobqueue = (new_jobqueue << 8) | new_job;

	/* Add the remaining jobs */
	for (; i >= 0; i -= 8) {
		next_job = (jobqueue >> i) & 0xFF;
		new_jobqueue = (new_jobqueue << 8) | next_job;
	}

	return new_jobqueue;
}

/**
 * pending list is only updated when sched is locked.
 * a session can only be added once
 *
 * notify_list = []
 * lock_sched
 * for pending in pending_list:
 *      if is_mapped(pending):
 *              jobqueue.add(pending)
 *              pending_list.remove(pending)
 *              continue
 *
 *      l = free_lsid
 *      if l is Nul:
 *              l = idle_lsid
 *              if l is Nul:
 *                      break
 *      if is_mapped(l):
 *              s = session[l]
 *              unmap(s)
 *              notify_list.add(s)
 *
 *      map(pending)
 *      jobqueue.add(pending)
 *      pending_list.remove(pending)
 * unlock_sched
 *
 * for s in notify_list:
 *      session_notify(s)
 *      notify_list.remove(s)
 */
static void sched_task(struct work_struct *ws)
{
	struct mvx_sched *sched =
	    container_of(ws, struct mvx_sched, sched_task);
	struct mvx_sched_session *pending;
	struct mvx_sched_session *unmapped;
	struct mvx_sched_session *tmp;
	LIST_HEAD(notify_list);
	int ret;

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0)
		return;

	if (sched->state == MVX_SCHED_STATE_IDLE) {
		if (list_empty_careful(&sched->pending) == false)
			set_sched_state(sched, MVX_SCHED_STATE_RUNNING);
	} else if (sched->state == MVX_SCHED_STATE_SUSPEND) {
		mutex_unlock(&sched->mutex);
		return;
	}

	/*
	 * Try to map sessions from pending queue while possible.
	 */
	list_for_each_entry_safe(pending, tmp, &sched->pending, pending) {
		struct mvx_lsid *lsid;

		/*
		 * This session is already mapped to LSID.
		 * Just make sure it is scheduled.
		 */
		if (pending->lsid != NULL) {
			ret = mvx_lsid_jobqueue_add(pending->lsid,
						    pending->isession->ncores,
						    sort_jobs);
			if (ret != 0) {
				MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
					      "Cannot add pending session to job queue. csession=%p, mvx_session=%p",
					      pending,
					      mvx_if_session_to_session
					      (pending->isession));
				continue;
			}

			pending->in_pending = false;
			list_del(&pending->pending);
			continue;
		}

		/* Find LSID to be used for the pending session. */
		lsid = find_free_lsid(sched);
		if (lsid == NULL)
			lsid = find_idle_lsid(sched);

		if (lsid == NULL)
			break;

		/*
		 * This LSID was mapped to some session. We have to notify
		 * the session about an irq in case there are messages in
		 * a message queue.
		 *
		 * Notifications are done after pending list is processed.
		 */
		if (lsid->session != NULL) {
			struct mvx_sched_session *unmapped = lsid->session;

			unmap_session(sched, unmapped);

			/*
			 * If the reference count is 0, then the session is
			 * about to be removed and should be ignored.
			 */
			ret = kref_get_unless_zero(&unmapped->isession->kref);
			if (ret != 0) {
				if (list_find_node(&notify_list,
						   &unmapped->notify))
					/*
					 * Consider a situation when a session
					 * that was unmapped from LSID and added
					 * notify_list was also present in the
					 * pending_list. It is possible that
					 * such a session will be mapped to the
					 * new LSID, executed by the hardware
					 * and switched to idle state while
					 * this function is still looping
					 * through pending list.
					 *
					 * If it happens, then this session
					 * might be unmapped again in order to
					 * make a room for another pending
					 * session. As a result we will try to
					 * add this session to notify_list
					 * again. This will break notify list
					 * and could lead to crashes or hangs.
					 *
					 * However, it is safe just to skip
					 * adding the session to notify_list if
					 * it is already there, because it will
					 * be processed anyway.
					 */
					kref_put(&unmapped->isession->kref,
						 unmapped->isession->release);
				else
					list_add_tail(&unmapped->notify,
						      &notify_list);
			} else {
				MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
					      "Ref is zero. csession=%p",
					      unmapped);
			}
		}

		ret = map_session(sched, pending, lsid);
		if (ret != 0) {
			MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
				      "Cannot map pending session. csession=%p, mvx_session=%p",
				      pending,
				      mvx_if_session_to_session
				      (pending->isession));
			break;
		}

		ret =
		    mvx_lsid_jobqueue_add(lsid, pending->isession->ncores,
					  sort_jobs);
		if (ret != 0) {
			MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
				      "Cannot add pending session to job queue. csession=%p, mvx_session=%p",
				      pending,
				      mvx_if_session_to_session
				      (pending->isession));
			continue;
		}

		pending->in_pending = false;
		list_del(&pending->pending);
	}

	/*
	 * It is important that the scheduler mutex is released before the
	 * callbacks to the if-module are invoked. The if-module may issue
	 * requests to the dev-module (for example switch_in()) that would
	 * otherwise deadlock.
	 */
	mutex_unlock(&sched->mutex);

	list_for_each_entry_safe(unmapped, tmp, &notify_list, notify) {
		struct mvx_if_session *iunmapped = unmapped->isession;

		list_del(&unmapped->notify);

		mutex_lock(iunmapped->mutex);
		sched->if_ops->irq(iunmapped);
		ret = kref_put(&iunmapped->kref, iunmapped->release);
		if (ret == 0)
			mutex_unlock(iunmapped->mutex);
	}
}

static void sched_session_print(struct seq_file *s,
				struct mvx_sched_session *session,
				struct mvx_hwreg *hwreg, int ind)
{
	struct mvx_lsid *lsid;

	if (session == NULL)
		return;

	mvx_seq_printf(s, "Client session", ind, "%px\n", session->isession);
	mvx_seq_printf(s, "Dev session", ind, "%px\n", session);
	mvx_seq_printf(s, "MVX session", ind, "%px\n",
		       mvx_if_session_to_session(session->isession));

	lsid = session->lsid;
	if (lsid == NULL)
		return;

	mvx_seq_printf(s, "IRQ host", ind, "%d\n",
		       mvx_hwreg_read_lsid(hwreg, lsid->lsid,
					   MVX_HWREG_IRQHOST));
	mvx_seq_printf(s, "IRQ MVE", ind, "%d\n",
		       mvx_hwreg_read_lsid(hwreg, lsid->lsid,
					   MVX_HWREG_LIRQVE));
}

static int sched_show(struct seq_file *s, void *v)
{
	struct mvx_sched *sched = (struct mvx_sched *)s->private;
	struct mvx_hwreg *hwreg = sched->hwreg;
	struct mvx_sched_session *session;
	int i;
	int ret;

	ret = mvx_pm_runtime_get_sync(hwreg->dev);
	if (ret < 0)
		return 0;

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0) {
		mvx_pm_runtime_put_sync(hwreg->dev);
		return ret;
	}

	mvx_seq_printf(s, "Core LSID", 0, "%08x\n",
		       mvx_hwreg_read(hwreg, MVX_HWREG_CORELSID));
	mvx_seq_printf(s, "Job queue", 0, "%08x\n",
		       mvx_hwreg_read(hwreg, MVX_HWREG_JOBQUEUE));
	seq_puts(s, "\n");

	seq_puts(s, "scheduled:\n");
	for (i = 0; i < sched->nlsid; ++i) {
		mvx_seq_printf(s, "LSID", 1, "%d\n", i);
		session = sched->lsid[i].session;
		sched_session_print(s, session, hwreg, 2);
	}

	seq_puts(s, "pending:\n");
	i = 0;
	list_for_each_entry(session, &sched->pending, pending) {
		char tmp[10];

		scnprintf(tmp, sizeof(tmp), "%d", i++);
		mvx_seq_printf(s, tmp, 1, "\n");
		sched_session_print(s, session, hwreg, 2);
	}

	mutex_unlock(&sched->mutex);
	mvx_pm_runtime_put_sync(hwreg->dev);

	return 0;
}

static int sched_open(struct inode *inode, struct file *file)
{
	return single_open(file, sched_show, inode->i_private);
}

static const struct file_operations sched_fops = {
	.open = sched_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

int sched_debugfs_init(struct mvx_sched *sched, struct dentry *parent)
{
	struct dentry *dentry;

	dentry = debugfs_create_file("sched", 0400, parent, sched, &sched_fops);
	if (IS_ERR_OR_NULL(dentry))
		return -ENOMEM;

	return 0;
}

/****************************************************************************
 * Exported functions
 ****************************************************************************/

int mvx_sched_construct(struct mvx_sched *sched,
			struct device *dev,
			struct mvx_if_ops *if_ops,
			struct mvx_hwreg *hwreg, struct dentry *parent)
{
	unsigned int lsid;
	int ret;

	sched->dev = dev;
	sched->hwreg = hwreg;
	sched->if_ops = if_ops;
	sched->state = MVX_SCHED_STATE_IDLE;
	init_completion(&sched->cmp);
	mutex_init(&sched->mutex);
	INIT_LIST_HEAD(&sched->pending);
	INIT_LIST_HEAD(&sched->sessions);
	mvx_log_perf.sessions = &sched->sessions;
	mutex_init(&sched->sessions_mutex);
	INIT_WORK(&sched->sched_task, sched_task);
	sched->sched_queue = create_singlethread_workqueue("mvx_sched");
	if (!sched->sched_queue) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
			      "Cannot create work queue");
		return -EINVAL;
	}

	sched->nlsid = mvx_hwreg_get_nlsid(hwreg);

	for (lsid = 0; lsid < sched->nlsid; lsid++) {
		ret = mvx_lsid_construct(&sched->lsid[lsid], dev, hwreg, lsid);
		if (ret != 0)
			goto destruct_lsid;
	}

	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		ret = sched_debugfs_init(sched, parent);
		if (ret != 0)
			goto destruct_lsid;
	}

	return 0;

destruct_lsid:
	while (lsid-- > 0)
		mvx_lsid_destruct(&sched->lsid[lsid]);

	return ret;
}

void mvx_sched_destruct(struct mvx_sched *sched)
{
	destroy_workqueue(sched->sched_queue);

	while (sched->nlsid-- > 0)
		mvx_lsid_destruct(&sched->lsid[sched->nlsid]);
}

int mvx_sched_session_construct(struct mvx_sched_session *session,
				struct mvx_if_session *isession)
{
	uint32_t disallow;
	uint32_t maxcores;
	struct mvx_session *s = mvx_if_session_to_session(isession);

	session->isession = isession;
	INIT_LIST_HEAD(&session->pending);
	INIT_LIST_HEAD(&session->notify);
	session->lsid = NULL;
	session->in_pending = false;

	memset(&session->pcb, 0, sizeof(session->pcb));

	disallow = (~isession->core_mask) & MVE_CTRL_DISALLOW_MASK;
	maxcores = isession->ncores & MVE_CTRL_MAXCORES_MASK;
	session->pcb.ctrl = (disallow << MVE_CTRL_DISALLOW_SHIFT) |
	    (maxcores << MVE_CTRL_MAXCORES_SHIFT);

	session->pcb.mmu_ctrl = isession->l0_pte;
	session->pcb.nprot = isession->securevideo == false;
	session->priority = s->priority;
	session->priority_in_queue = session->priority_pending =
	    session->priority;

	return 0;
}

void mvx_sched_session_destruct(struct mvx_sched_session *session)
{
}

void mvx_sched_list_insert_by_priority(struct mvx_sched *sched,
				       struct mvx_sched_session *session)
{
	struct mvx_sched_session *tmp;

	/* To minimize impact on no priority case, use reverse iteration */
	list_for_each_entry_reverse(tmp, &sched->pending, pending) {
		if (session->priority >= tmp->priority_pending) {
			list_add(&session->pending, &tmp->pending);
			return;
		} else if (tmp->priority_pending > 1 && session->priority > 0) {
			tmp->priority_pending--;
		}
	}

	list_add(&session->pending, &sched->pending);
}

int mvx_sched_switch_in(struct mvx_sched *sched,
			struct mvx_sched_session *session)
{
	int ret;

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
		      "%p Switch in session. jobqueue=%08x, coreslid=%08x.",
		      mvx_if_session_to_session(session->isession),
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_JOBQUEUE),
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_CORELSID));

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0)
		return ret;

	if (session->in_pending) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_VERBOSE,
			      "Session is already in pending state.");
		goto unlock_mutex;
	}

	session->in_pending = true;
	session->priority_pending = session->priority;
	mvx_sched_list_insert_by_priority(sched, session);
	queue_work(sched->sched_queue, &sched->sched_task);

unlock_mutex:
	mutex_unlock(&sched->mutex);
	return 0;
}

int mvx_sched_switch_out_rsp(struct mvx_sched *sched,
			     struct mvx_sched_session *session)
{
	int i;
	int ret;
	bool all_lsid_idle = true;

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
			      "get scheduler lock fail.");
		return ret;
	}

	for (i = 0; i < sched->nlsid; i++)
		all_lsid_idle &= mvx_lsid_idle(&sched->lsid[i]);

	for (i = 0; i < sched->nlsid; i++) {
		struct mvx_sched_session *ss = sched->lsid[i].session;
		struct mvx_session *ls = NULL;

		if (ss != NULL) {
			ls = mvx_if_session_to_session(ss->isession);
			all_lsid_idle &= ls ? !ls->switched_in : true;
		}
	}

	if (sched->state == MVX_SCHED_STATE_SUSPEND) {
		if (all_lsid_idle == false)
			goto end;

		complete(&sched->cmp);
	} else if (sched->state == MVX_SCHED_STATE_RUNNING) {
		if (list_empty_careful(&sched->pending)
		    && all_lsid_idle == true)
			set_sched_state(sched, MVX_SCHED_STATE_IDLE);
	}

end:
	mutex_unlock(&sched->mutex);
	return ret;
}

int mvx_sched_send_irq(struct mvx_sched *sched,
		       struct mvx_sched_session *session)
{
	mutex_lock(&sched->mutex);

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_DEBUG,
		      "%p Send irq. lsid=%d, jobqueue=%08x, corelsid=%08x.",
		      mvx_if_session_to_session(session->isession),
		      session->lsid == NULL ? -1 : session->lsid->lsid,
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_JOBQUEUE),
		      mvx_hwreg_read(sched->hwreg, MVX_HWREG_CORELSID));

	if (session->lsid == NULL)
		session->pcb.irqhost = 1;
	else
		mvx_lsid_send_irq(session->lsid);

	mutex_unlock(&sched->mutex);

	return 0;
}

int mvx_sched_trigger_irq(struct mvx_sched *sched,
			  struct mvx_sched_session *session)
{
	mutex_lock(&sched->mutex);

	if (session->lsid == NULL) {
		mutex_unlock(&sched->mutex);
		return 0;
	}

	mvx_lsid_trigger_irqve(session->lsid);
	mvx_lsid_jobqueue_add(session->lsid,
			      session->lsid->session->isession->ncores,
			      sort_jobs);
	mvx_lsid_send_irq(session->lsid);

	mutex_unlock(&sched->mutex);

	return 0;
}

int mvx_sched_flush_mmu(struct mvx_sched *sched,
			struct mvx_sched_session *session)
{
	mutex_lock(&sched->mutex);

	if (session->lsid != NULL)
		mvx_lsid_flush_mmu(session->lsid);

	mutex_unlock(&sched->mutex);

	return 0;
}

static void print_session(struct mvx_sched *sched,
			  struct mvx_sched_session *session,
			  struct mvx_session *s)
{
	int lsid = -1;
	uint32_t irqve = 0;
	uint32_t irqhost = 0;

	if (session != NULL && session->lsid != NULL) {
		struct mvx_hwreg *hwreg = sched->hwreg;

		lsid = session->lsid->lsid;
		irqve = mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_LIRQVE);
		irqhost = mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_IRQHOST);
	}

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
		      "%p    session=%p, lsid=%d, irqve=%08x, irqhost=%08x",
		      s, mvx_if_session_to_session(session->isession), lsid,
		      irqve, irqhost);
}

void mvx_sched_print_debug(struct mvx_sched *sched,
			   struct mvx_sched_session *session)
{
	struct mvx_hwreg *hwreg = sched->hwreg;
	struct mvx_sched_session *pending;
	struct mvx_sched_session *tmp;
	struct mvx_session *s = mvx_if_session_to_session(session->isession);
	unsigned int i;
	int ret;

	mvx_pm_runtime_get_sync(sched->dev);

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0) {
		mvx_pm_runtime_put_sync(sched->dev);
		return;
	}

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING, "%p Current session:", s);
	print_session(sched, session, s);

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING, "%p Pending queue:", s);
	list_for_each_entry_safe(pending, tmp, &sched->pending, pending) {
		print_session(sched, pending, s);
	}

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING, "%p Print register:", s);

	MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
		      "%p     jobqueue=%08x, corelsid=%08x, irqve=%08x",
		      s,
		      mvx_hwreg_read(hwreg, MVX_HWREG_JOBQUEUE),
		      mvx_hwreg_read(hwreg, MVX_HWREG_CORELSID),
		      mvx_hwreg_read(hwreg, MVX_HWREG_IRQVE));

	for (i = 0; i < sched->nlsid; i++) {
		struct mvx_sched_session *ss = sched->lsid[i].session;
		struct mvx_session *ls = NULL;

		if (ss != NULL)
			ls = mvx_if_session_to_session(ss->isession);

		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
			      "%p     lsid=%u, session=%p, irqve=%08x, irqhost=%08x",
			      s, i, ls,
			      mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_LIRQVE),
			      mvx_hwreg_read_lsid(hwreg, i, MVX_HWREG_IRQHOST));
	}

	mutex_unlock(&sched->mutex);

	mvx_pm_runtime_put_sync(sched->dev);
}

void mvx_sched_handle_irq(struct mvx_sched *sched, unsigned int lsid)
{
	struct mvx_sched_session *session;
	struct mvx_if_session *isession = NULL;
	int ret;

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0)
		return;

	/*
	 * If a session has been terminated/unmapped just before the IRQ bottom
	 * handler has been executed, then the session pointer will be NULL or
	 * may even point at a different session. This is an unharmful
	 * situation.
	 *
	 * If the reference count is 0, then the session is about to be removed
	 * and should be ignored.
	 */
	session = sched->lsid[lsid].session;
	if (session != NULL) {
		ret = kref_get_unless_zero(&session->isession->kref);
		if (ret != 0)
			isession = session->isession;
	} else {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
			      "lsid[%d].session has been terminated or unmapped",
			      lsid);
	}

	/*
	 * It is important that the scheduler mutex is released before the
	 * callbacks to the if-module are invoked. The if-module may issue
	 * requests to the dev-module (for example switch_in()) that would
	 * otherwise deadlock.
	 */
	mutex_unlock(&sched->mutex);

	/* Inform if-session that an IRQ was received. */
	if (isession != NULL) {
		mutex_lock(isession->mutex);
		sched->if_ops->irq(isession);
		ret = kref_put(&isession->kref, isession->release);

		if (ret == 0)
			mutex_unlock(isession->mutex);
	}

	queue_work(sched->sched_queue, &sched->sched_task);
}

void mvx_sched_terminate(struct mvx_sched *sched,
			 struct mvx_sched_session *session)
{
	struct list_head *head;
	struct list_head *tmp;

	mutex_lock(&sched->mutex);

	if (session->lsid != NULL) {
		mvx_lsid_jobqueue_remove(session->lsid);
		mvx_lsid_terminate(session->lsid);
		unmap_session(sched, session);
	}

	list_for_each_safe(head, tmp, &sched->pending) {
		if (head == &session->pending) {
			list_del(head);
			break;
		}
	}

	mutex_unlock(&sched->mutex);
}

void mvx_sched_reset_priority(struct mvx_sched *sched,
			      struct mvx_sched_session *session)
{
	mutex_lock(&sched->mutex);
	session->priority_in_queue = session->priority;
	mutex_unlock(&sched->mutex);
}

static int mvx_sched_cancel_session_work(struct mvx_sched *sched)
{
	struct mvx_sched_session *session;
	struct mvx_sched_session *tmp;

	mutex_lock(&sched->sessions_mutex);

	list_for_each_entry_safe(session, tmp, &sched->sessions, session) {
		if (session && session->isession)
			mvx_session_cancel_work(mvx_if_session_to_session
						(session->isession));
	}

	mutex_unlock(&sched->sessions_mutex);

	return 0;
}

int mvx_sched_suspend(struct mvx_sched *sched)
{
	int ret;
	int i;
	bool wait_suspend = false;

	for (i = 0; i < sched->nlsid; i++) {
		struct mvx_sched_session *ss = sched->lsid[i].session;
		struct mvx_session *ls = NULL;

		if (ss != NULL) {
			ls = mvx_if_session_to_session(ss->isession);
			if (ls && ls->job_frames == 0 && ls->switched_in) {
				MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
					      "lsid[%d] insert switch-out when suspend.",
					      i);
				mvx_session_switch_out(ls);
			}
		}
	}

	reinit_completion(&sched->cmp);
	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
			      "Get scheduler lock fail.");
		return -EBUSY;
	}

	if (sched->state == MVX_SCHED_STATE_RUNNING)
		wait_suspend = true;
	set_sched_state(sched, MVX_SCHED_STATE_SUSPEND);
	mutex_unlock(&sched->mutex);

	if (wait_suspend) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_INFO,
			      "Waiting scheduler idle.");
		ret =
		    wait_for_completion_timeout(&sched->cmp,
						msecs_to_jiffies
						(wait_scheduler_timeout));
		if (!ret)
			MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
				      "Waiting scheduler idle timeout.");
	}

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
			      "Get scheduler lock fail.");
		return -EBUSY;
	}

	for (i = 0; i < MVX_LSID_MAX; i++)
		if (sched->lsid[i].session)
			unmap_session(sched, sched->lsid[i].session);

	mutex_unlock(&sched->mutex);

	return ret;
}

int mvx_sched_resume(struct mvx_sched *sched)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(sched->dev))
		return ret;

	ret = mutex_lock_interruptible(&sched->mutex);
	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
			      "get scheduler lock fail.");
		return ret;
	}
	set_sched_state(sched, MVX_SCHED_STATE_IDLE);
	mutex_unlock(&sched->mutex);

	queue_work(sched->sched_queue, &sched->sched_task);

	return ret;
}

static unsigned long calculate_session_load(struct mvx_session *session)
{
	unsigned long mbs;
	struct mvx_session_port *port_in = &session->port[MVX_DIR_INPUT];
	struct mvx_session_port *port_out = &session->port[MVX_DIR_OUTPUT];
	uint32_t fps;
	uint32_t buf_fps;
	struct timespec64 now;
	struct timespec64 delta;

	if (session->fw_state == MVX_FW_STATE_STOPPED)
		return 0;

	mbs =
	    (ALIGN(session->orig_width, 16) / 16) *
	    (ALIGN(session->orig_height, 16) / 16);

	ktime_get_real_ts64(&now);
	delta = timespec64_sub(now, session->last_timespec);

	/*
	 * If actual fps exceeds 20% of the setting, assume current setting fps is low,
	 * update work load base on actual fps to meet performance requirement.
	 * The sampling interval is 0.5s.
	 * It is usually used in DFS simple_ondemand mode.
	 */
	if (timespec64_to_ns(&delta) / NSEC_PER_MSEC > 500) {
		buf_fps =
		    port_in->buffers_in_window * NSEC_PER_SEC /
		    timespec64_to_ns(&delta);
		fps =
		    (buf_fps >
		     ((session->fps_n / session->fps_d) * 120 /
		      100)) ? buf_fps : (session->fps_n / session->fps_d);

		port_in->buffers_in_window = 0;
		port_out->buffers_in_window = 0;
		session->last_timespec = now;
		session->last_fps = fps;
	} else {
		fps = max(session->last_fps, session->fps_n / session->fps_d);
	}

	// The performance of encode is half that of decode, we use decode as the benchmark.
	if (session->is_encoder) {
		fps *= 2;
		if (port_out->format == MVX_FORMAT_VP8)
			fps = fps * 4 / 3;	// VP8 encode is 1.33x slower
	} else {
		if (MVX_IS_LEGACY_FORMAT(port_in->format))
			fps = fps * 8 / 3;	// Legacy formats are 2.67x slower
	}

	return mbs * fps;
}

int mvx_sched_calculate_load(struct mvx_sched *sched,
			     unsigned long *mbs_per_sec)
{
	struct mvx_sched_session *session;
	struct mvx_sched_session *tmp;

	if (IS_ERR_OR_NULL(&sched->sessions))
		return -EINVAL;

	if (!mutex_trylock(&sched->sessions_mutex))
		return -EBUSY;

	*mbs_per_sec = 0;
	list_for_each_entry_safe(session, tmp, &sched->sessions, session) {
		if (session && session->isession)
			*mbs_per_sec +=
			    calculate_session_load(mvx_if_session_to_session
						   (session->isession));
	}

	mutex_unlock(&sched->sessions_mutex);

	return 0;
}

static void update_session_job_frames(struct mvx_sched *sched,
				      uint32_t job_frames)
{
	struct mvx_sched_session *session;
	struct mvx_sched_session *tmp;

	list_for_each_entry_safe(session, tmp, &sched->sessions, session) {
		if (session && session->isession) {
			struct mvx_session *s =
			    mvx_if_session_to_session(session->isession);
			if (s->job_frames == 0 && job_frames == 1)
				s->pending_switch_out = true;
			s->job_frames = job_frames;
		}
	}
}

int mvx_sched_add_session(struct mvx_sched *sched, struct list_head *session)
{
	int ret = 0;

	ret = mutex_lock_interruptible(&sched->sessions_mutex);
	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_WARNING,
			      "get scheduler lock fail.");
		return ret;
	}

	list_add_tail(session, &sched->sessions);
	sched->session_count++;
	if (sched->session_count <= 2) {
		uint32_t job_frames = sched->session_count == 1 ? 0 : 1;

		update_session_job_frames(sched, job_frames);
	}

	mutex_unlock(&sched->sessions_mutex);

	return 0;
}

int mvx_sched_remove_session(struct mvx_sched *sched, struct list_head *session)
{
	mutex_lock(&sched->sessions_mutex);

	if (session->prev != NULL && session->next != NULL
	    && sched->session_count > 0) {
		list_del(session);
		sched->session_count--;
		if (sched->session_count == 1)
			update_session_job_frames(sched, 0);
	}

	mutex_unlock(&sched->sessions_mutex);

	return 0;
}

bool mvx_sched_sessions_empty(struct mvx_sched *sched)
{
	return list_empty_careful(&sched->sessions);
}

int mvx_sched_cancel_work(struct mvx_sched *sched)
{
	int ret;

	ret = mvx_sched_cancel_session_work(sched);
	if (ret != 0)
		return ret;

	cancel_work_sync(&sched->sched_task);

	return ret;
}

void mvx_sched_get_realtime_fps(struct list_head *sessions)
{
	struct mvx_sched_session *session;
	struct mvx_sched_session *tmp;
	struct mvx_sched *sched =
	    container_of(sessions, struct mvx_sched, sessions);

	mutex_lock(&sched->sessions_mutex);

	list_for_each_entry_safe(session, tmp, sessions, session) {
		if (session && session->isession) {
			struct mvx_session *s =
			    mvx_if_session_to_session(session->isession);
			mvx_session_update_realtime_fps(s);
		}
	}

	mutex_unlock(&sched->sessions_mutex);
}
