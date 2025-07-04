#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/reboot.h>
#include <linux/cciss_ioctl.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_transport_sas.h>
#include <scsi/scsi_dbg.h>
#include <asm/unaligned.h>
#include "trustrlib.h"

#ifdef KFEATURE_ENABLE_TRUSTRLIB
#ifdef	CONFIG_LOONGSON
#include <asm/cpu-info.h>
#else	/* CONFIG_SW */
#include <asm/cache.h>
#include <asm/cpu.h>
#include <asm/mmu_context.h>
#endif
#endif

#if !defined(BUILD_TIMESTAMP)
#define BUILD_TIMESTAMP
#endif

#define DRIVER_VERSION		"1.3.0"
#define DRIVER_MAJOR		1
#define DRIVER_MINOR		3
#define DRIVER_RELEASE		0

#define DRIVER_NAME		"TrustRaid D3152s Library (v" \
				DRIVER_VERSION BUILD_TIMESTAMP ")"
#define DRIVER_NAME_SHORT	"trustrlib"

#if !defined(PCI_VENDOR_ID_POWERLEADER)
#define PCI_VENDOR_ID_POWERLEADER		0x1f3a
#endif

#if !defined(PCI_VENDOR_ID_HRDT)
#define PCI_VENDOR_ID_HRDT	0x207d
#endif

static int trustrlib_debug;
module_param_named(debug,
	trustrlib_debug, int, 0644);
MODULE_PARM_DESC(debug,
	"Debug SCSI commands.");

static int trustrlib_disable_poll_complete_queue;
module_param_named(disable_poll_complete_queue,
	trustrlib_disable_poll_complete_queue, uint, 0644);
MODULE_PARM_DESC(disable_poll_complete_queue,
	"Disable poll complete queue.");

#define	PQI_POLL_COMPLETE_QUEUE_TIMER_INTERVAL	(10 * HZ)
#define PQI_RECHECK_IQ_HANG_DELAY	(10UL * 60 * HZ)
#define TRUSTRLIB_WAIT_FOR_COMPLETION_IO_TIMEOUT_SECS	10
#define PQI_SCSI_REQ_BUFLEN		512

void trustrlib_start_io(void *p1, void *p2, int path);
u16 trustrlib_get_and_update_io_request(void *p1, void *p2);
void trustrlib_free_io_request(void *p);

static bool pqi_io_request_referenced(struct pqi_io_request *io_request)
{
	unsigned long flags;
	bool referenced;
	struct trustrlib_io_request *t = (struct trustrlib_io_request *)io_request->trustrlib_data;

	spin_lock_irqsave(&t->lock, flags);
	referenced = !(atomic_read(&io_request->refcount) == 0);
	spin_unlock_irqrestore(&t->lock, flags);

	return referenced;
}

static int __trustrlib_process_io_intr(struct pqi_ctrl_info *ctrl_info,
	struct pqi_queue_group *queue_group)
{
	struct trustrlib_ctrl_info *trustrlib = (struct trustrlib_ctrl_info *)ctrl_info->trustrlib_data;
	struct trustrlib_template *template = trustrlib->template;
	struct trustrlib_queue_group *trustrlib_queue;
	int num_responses;
	pqi_index_t oq_pi;
	pqi_index_t oq_ci;
	struct pqi_io_request *io_request;
	struct trustrlib_io_request *t;
	struct pqi_io_response *response;
	u16 request_id, rid;
	pqi_index_t last_unmatch_ci = 0;
	pqi_index_t oq_pi_next;
	int trycount = 0;
	int loop_count = 0;
	int inflight_num;

	num_responses = 0;
	oq_ci = queue_group->oq_ci_copy;
	oq_pi = readl(queue_group->oq_pi);
	last_unmatch_ci = ctrl_info->num_elements_per_oq;

	if (oq_pi >= ctrl_info->num_elements_per_oq) {
		template->invalid_response(ctrl_info, PQI_IO_PI_OUT_OF_RANGE);
		dev_err(&ctrl_info->pci_dev->dev,
			"I/O interrupt: producer index (%u) out of range (0-%u): consumer index: %u\n",
			oq_pi, ctrl_info->num_elements_per_oq - 1, oq_ci);
		return -1;
	}

	while (1) {
		if (loop_count++ > 1000) {
			dev_err(&ctrl_info->pci_dev->dev, "loop limits (%d): num_responses %d\n",
				loop_count, num_responses);
			break;
		}

		if (oq_pi == oq_ci)
			break;

		num_responses++;
		response = queue_group->oq_element_array + (oq_ci * PQI_OPERATIONAL_OQ_ELEMENT_LENGTH);

		request_id = get_unaligned_le16(&response->request_id);
		if ((request_id & PQI_IO_REQUEST_INDEX_MASK) >= ctrl_info->max_io_slots) {
			template->invalid_response(ctrl_info, PQI_INVALID_REQ_ID);
			dev_err(&ctrl_info->pci_dev->dev,
				"request ID in response (%u) out of range (0-%u): producer index: %u  consumer index: %u\n",
				request_id, ctrl_info->max_io_slots - 1, oq_pi, oq_ci);
			return -1;
		}

		io_request = &ctrl_info->io_request_pool[request_id & PQI_IO_REQUEST_INDEX_MASK];
		if (io_request->index != request_id || !pqi_io_request_referenced(io_request)) {
			// template->invalid_response(ctrl_info, PQI_UNMATCHED_REQ_ID);
			if (trustrlib_debug > 0) {
				dev_err(&ctrl_info->pci_dev->dev,
					"request ID in response (%u) does not match an outstanding I/O request: producer index: %u  consumer index: %u\n",
					request_id, oq_pi, oq_ci);
				dev_info(&ctrl_info->pci_dev->dev,
					"queue %p io_request %p: index %x request_id %x refcount %d scmd %p io_complete_callback %p\n",
					queue_group, io_request, io_request->index, request_id, atomic_read(&io_request->refcount),
					io_request->scmd, io_request->io_complete_callback);
			}

			smp_mb();
			rid = get_unaligned_le16(&response->request_id);
			if (rid != request_id) {
				trycount = 0;
				if (trustrlib_debug > 0) {
					dev_info(&ctrl_info->pci_dev->dev, "request_id changed: %x -> %x\n",
						request_id, rid);
					io_request = &ctrl_info->io_request_pool[rid & PQI_IO_REQUEST_INDEX_MASK];
					dev_info(&ctrl_info->pci_dev->dev,
						"queue %p io_request %p: index %x refcount %d scmd %p io_complete_callback %p\n",
						queue_group, io_request, io_request->index, atomic_read(&io_request->refcount),
						io_request->scmd, io_request->io_complete_callback);
				}

				if (oq_ci == last_unmatch_ci) {
					dev_err(&ctrl_info->pci_dev->dev, "request_id changed twice: %x -> %x\n",
						request_id, rid);
					dev_info(&ctrl_info->pci_dev->dev,
						"queue %p io_request %p: index %x request_id %x refcount %d scmd %p io_complete_callback %p\n",
						queue_group, io_request, io_request->index, request_id, atomic_read(&io_request->refcount),
						io_request->scmd, io_request->io_complete_callback);
					oq_ci = (oq_ci + 1) % ctrl_info->num_elements_per_oq;
					continue;
				}
				last_unmatch_ci = oq_ci;
			} else if (++trycount > 3) {
				oq_pi_next = readl(queue_group->oq_pi);
				if (trustrlib_debug > 0) {
					dev_info(&ctrl_info->pci_dev->dev,
						"try count limit, request ID in response (%x) does not match an outstanding I/O request: producer index: %u consumer index: %u oq_ci_copy %u oq_pi_next %u\n",
						request_id, oq_pi, oq_ci, queue_group->oq_ci_copy, oq_pi_next);
					dev_info(&ctrl_info->pci_dev->dev,
						"queue %p io_request %p: index %x request_id %x refcount %d scmd %p io_complete_callback %p\n",
						queue_group, io_request, io_request->index, request_id, atomic_read(&io_request->refcount),
						io_request->scmd, io_request->io_complete_callback);
				}
				if (oq_pi_next != oq_pi) {
					oq_pi = oq_pi_next;
					continue;
				}
				trycount = 0;
				oq_ci = (oq_ci + 1) % ctrl_info->num_elements_per_oq;
				continue;
			}

			num_responses--;
			continue;
		}

		t = (struct trustrlib_io_request *)io_request->trustrlib_data;
		if (atomic_inc_return(&t->in_interrupt_or_timedout) > 1) {
			atomic_dec(&t->in_interrupt_or_timedout);
			if (trustrlib_debug > 0) {
				dev_info(&ctrl_info->pci_dev->dev,
					"I/O interrupt: io_request %p timedout\n",
					io_request);
			}
			return -1;
		}

		if (template->handle_response(ctrl_info, io_request, response) != 0)
			return -1;

		/*
		 * Note that the I/O request structure CANNOT BE TOUCHED after
		 * returning from the I/O completion callback!
		 */
		oq_ci = (oq_ci + 1) % ctrl_info->num_elements_per_oq;
	}

	if (oq_ci != queue_group->oq_ci_copy) {
		queue_group->oq_ci_copy = oq_ci;
		writel(oq_ci, queue_group->oq_ci);

		trustrlib_queue = (struct trustrlib_queue_group *)queue_group->trustrlib_data;
		inflight_num = atomic_sub_return(num_responses, &trustrlib_queue->inflight_num);
		if (inflight_num < 0) {
			if (trustrlib_debug) {
				dev_info(&ctrl_info->pci_dev->dev,
					"I/O interrupt: queue_group %p inflight_num %d -%d\n",
					queue_group, inflight_num, num_responses);
			}
			atomic_add(-inflight_num, &trustrlib_queue->inflight_num);
		}
	}

	return num_responses;
}

int trustrlib_process_io_intr(void *p1, void *p2)
{
	struct pqi_ctrl_info *ctrl_info = (struct pqi_ctrl_info *)p1;
	struct pqi_queue_group *queue_group = (struct pqi_queue_group *)p2;
	struct trustrlib_queue_group *t = (struct trustrlib_queue_group *)queue_group->trustrlib_data;
	int rc;

	spin_lock(&t->complete_lock);
	rc = __trustrlib_process_io_intr(ctrl_info, queue_group);
	spin_unlock(&t->complete_lock);

	return rc;
}

static inline unsigned int __pqi_num_elements_free(unsigned int pi,
	unsigned int ci, unsigned int elements_in_queue)
{
	unsigned int num_elements_used;

	if (pi >= ci)
		num_elements_used = pi - ci;
	else
		num_elements_used = elements_in_queue - ci + pi;

	return elements_in_queue - num_elements_used - 1;
}

static void pqi_poll_complete_queue(struct pqi_queue_group *queue_group)
{
	struct pqi_ctrl_info *ctrl_info;
	struct trustrlib_queue_group *t;
	unsigned long flags;
	int inflight_num, num_response;

	ctrl_info = queue_group->ctrl_info;
	t = (struct trustrlib_queue_group *)queue_group->trustrlib_data;

	inflight_num = atomic_read(&t->inflight_num);
	if (inflight_num == 0)
		return;
	if (inflight_num < 0) {
		dev_err(&ctrl_info->pci_dev->dev,
			"pqi_poll_complete_queue: unexpected inflight_num %d, queue_group %p\n",
			inflight_num, queue_group);
		// BUG();
	}

	if (!spin_trylock_irqsave(&t->complete_lock, flags))
		return;

	if (trustrlib_debug > 0) {
		dev_info(&ctrl_info->pci_dev->dev, "poll queue_group %p inflight_num %d\n",
			queue_group, inflight_num);
	}
	num_response = __trustrlib_process_io_intr(ctrl_info, queue_group);
	if (trustrlib_debug > 0) {
		dev_info(&ctrl_info->pci_dev->dev, "poll queue_group %p num_response %d\n",
			queue_group, num_response);
	}
	spin_unlock_irqrestore(&t->complete_lock, flags);

	spin_lock_irqsave(&queue_group->submit_lock[0], flags);
	trustrlib_start_io(ctrl_info, queue_group, 0);
	spin_unlock_irqrestore(&queue_group->submit_lock[0], flags);
	spin_lock_irqsave(&queue_group->submit_lock[1], flags);
	trustrlib_start_io(ctrl_info, queue_group, 1);
	spin_unlock_irqrestore(&queue_group->submit_lock[1], flags);
}

static void pqi_poll_complete_queue_timer_handler(struct timer_list *t)
{
	struct trustrlib_ctrl_info *t_ctrl;
	struct pqi_ctrl_info *ctrl_info;
	unsigned int i;

	if (trustrlib_disable_poll_complete_queue)
		return;

	t_ctrl = from_timer(t_ctrl, t, poll_complete_queue_timer);
	ctrl_info = t_ctrl->ctrl_info;

	if (!ctrl_info->controller_online)
		return;

	for (i = 0; i < ctrl_info->num_queue_groups; i++) {
		pqi_poll_complete_queue(&ctrl_info->queue_groups[i]);
	}

	mod_timer(&t_ctrl->poll_complete_queue_timer, jiffies + PQI_POLL_COMPLETE_QUEUE_TIMER_INTERVAL);
}

/* like pqi_start_io, but dont add to request_list */
static void pqi_cut_in_line(struct pqi_ctrl_info *ctrl_info,
	struct pqi_queue_group *queue_group, enum pqi_io_path path,
	struct pqi_io_request *io_request)
{
	void *next_element;
	pqi_index_t iq_pi;
	pqi_index_t iq_ci;
	size_t iu_length;
	unsigned long flags;
	unsigned int num_elements_needed;
	unsigned int num_elements_to_end_of_queue;
	size_t copy_count;
	struct pqi_iu_header *request;
	struct trustrlib_queue_group *t = (struct trustrlib_queue_group *)queue_group->trustrlib_data;

	spin_lock_irqsave(&queue_group->submit_lock[path], flags);

	iq_pi = queue_group->iq_pi_copy[path];

	request = io_request->iu;

	iu_length = get_unaligned_le16(&request->iu_length) + PQI_REQUEST_HEADER_LENGTH;
	num_elements_needed = DIV_ROUND_UP(iu_length, PQI_OPERATIONAL_IQ_ELEMENT_LENGTH);

	iq_ci = readl(queue_group->iq_ci[path]);

	if (num_elements_needed > __pqi_num_elements_free(iq_pi, iq_ci, ctrl_info->num_elements_per_iq)) {
		spin_unlock_irqrestore(&queue_group->submit_lock[path], flags);
		dev_err(&ctrl_info->pci_dev->dev,
			"pqi_cut_in_line: inqueue fail, pi %u ci %u num_elements_needed %u\n",
			iq_pi, iq_ci, num_elements_needed);
		return;
	}

	put_unaligned_le16(queue_group->oq_id, &request->response_queue_id);

	next_element = queue_group->iq_element_array[path] +
		(iq_pi * PQI_OPERATIONAL_IQ_ELEMENT_LENGTH);

	num_elements_to_end_of_queue = ctrl_info->num_elements_per_iq - iq_pi;

	if (num_elements_needed <= num_elements_to_end_of_queue) {
		memcpy(next_element, request, iu_length);
	} else {
		copy_count = num_elements_to_end_of_queue * PQI_OPERATIONAL_IQ_ELEMENT_LENGTH;
		memcpy(next_element, request, copy_count);
		memcpy(queue_group->iq_element_array[path],
			(u8 *)request + copy_count,
			iu_length - copy_count);
	}

	iq_pi = (iq_pi + num_elements_needed) % ctrl_info->num_elements_per_iq;

	atomic_add_return(1, &t->inflight_num);

	queue_group->iq_pi_copy[path] = iq_pi;
	writel(iq_pi, queue_group->iq_pi[path]);

	spin_unlock_irqrestore(&queue_group->submit_lock[path], flags);
}

static int __pqi_map_single(struct pci_dev *pci_dev,
	struct pqi_sg_descriptor *sg_descriptor, void *buffer,
	size_t buffer_length, enum dma_data_direction data_direction)
{
	dma_addr_t bus_address;

	if (!buffer || buffer_length == 0 || data_direction == DMA_NONE)
		return 0;

	bus_address = dma_map_single(&pci_dev->dev, buffer, buffer_length, data_direction);
	if (dma_mapping_error(&pci_dev->dev, bus_address))
		return -ENOMEM;

	put_unaligned_le64((u64)bus_address, &sg_descriptor->address);
	put_unaligned_le32(buffer_length, &sg_descriptor->length);
	put_unaligned_le32(CISS_SG_LAST, &sg_descriptor->flags);

	return 0;
}

static void __pqi_pci_unmap(struct pci_dev *pci_dev,
	struct pqi_sg_descriptor *descriptors, int num_descriptors,
	enum dma_data_direction data_direction)
{
	int i;

	if (data_direction == DMA_NONE)
		return;

	for (i = 0; i < num_descriptors; i++)
		dma_unmap_single(&pci_dev->dev,
			(dma_addr_t)get_unaligned_le64(&descriptors[i].address),
			get_unaligned_le32(&descriptors[i].length),
			data_direction);
}

static int pqi_build_inquiry_raid_path_request(struct pqi_ctrl_info *ctrl_info,
	struct pqi_raid_path_request *request,
	u8 *scsi3addr, void *buffer, size_t buffer_length)
{
	u8 *cdb;
	size_t cdb_length = buffer_length;

	memset(request, 0, sizeof(*request));

	request->header.iu_type = PQI_REQUEST_IU_RAID_PATH_IO;
	put_unaligned_le16(offsetof(struct pqi_raid_path_request,
		sg_descriptors[1]) - PQI_REQUEST_HEADER_LENGTH,
		&request->header.iu_length);
	put_unaligned_le32(buffer_length, &request->buffer_length);
	memcpy(request->lun_number, scsi3addr, sizeof(request->lun_number));
	request->task_attribute = SOP_TASK_ATTRIBUTE_SIMPLE;
	request->additional_cdb_bytes_usage = SOP_ADDITIONAL_CDB_BYTES_0;

	cdb = request->cdb;

	request->data_direction = SOP_READ_FLAG;
	cdb[0] = INQUIRY;
	cdb[4] = (u8)cdb_length;

	return __pqi_map_single(ctrl_info->pci_dev, &request->sg_descriptors[0],
		buffer, buffer_length, DMA_FROM_DEVICE);
}

static int pqi_build_inquiry_aio_path_request(struct pqi_ctrl_info *ctrl_info,
	struct pqi_aio_path_request *request,
	u8 *scsi3addr, void *buffer, size_t buffer_length)
{
	u8 *cdb;
	size_t cdb_length = buffer_length;

	memset(request, 0, sizeof(*request));

	request->header.iu_type = PQI_REQUEST_IU_AIO_PATH_IO;
	put_unaligned_le16(offsetof(struct pqi_aio_path_request,
		sg_descriptors[1]) - PQI_REQUEST_HEADER_LENGTH,
		&request->header.iu_length);
	put_unaligned_le32(buffer_length, &request->buffer_length);
	memcpy(request->lun_number, scsi3addr, sizeof(request->lun_number));
	request->task_attribute = SOP_TASK_ATTRIBUTE_SIMPLE;

	cdb = request->cdb;

	request->data_direction = SOP_READ_FLAG;
	cdb[0] = INQUIRY;
	// cdb[1] = 0x1;
	// cdb[2] = (u8)(VPD_PAGE | SCSI_VPD_DEVICE_ID);
	cdb[4] = (u8)cdb_length;
	request->cdb_length = 6;

	return __pqi_map_single(ctrl_info->pci_dev, &request->sg_descriptors[0],
		buffer, buffer_length, DMA_FROM_DEVICE);
}

static int __pqi_process_raid_io_error_synchronous(
	struct pqi_raid_error_info *error_info)
{
	int rc = -EIO;

	switch (error_info->data_out_result) {
	case PQI_DATA_IN_OUT_GOOD:
		if (error_info->status == SAM_STAT_GOOD)
			rc = 0;
		break;
	case PQI_DATA_IN_OUT_UNDERFLOW:
		if (error_info->status == SAM_STAT_GOOD || error_info->status == SAM_STAT_CHECK_CONDITION)
			rc = 0;
		break;
	case PQI_DATA_IN_OUT_ABORTED:
		rc = PQI_CMD_STATUS_ABORTED;
		break;
	}

	return rc;
}

static void pqi_check_inquiry_complete(struct pqi_io_request *io_request,
	void *context)
{
	struct completion *waiting = context;
	complete(waiting);
}

static int pqi_submit_check_inquiry(struct pqi_ctrl_info *ctrl_info,
	struct pqi_queue_group *queue_group, enum pqi_io_path path)
{
	int rc = 0;
	struct pqi_io_request *io_request = NULL;
	struct pqi_iu_header *request;
	struct pqi_raid_path_request *raid_request = NULL;
	struct pqi_aio_path_request *aio_request = NULL;
	u8 *buffer = NULL;
	struct completion *wait = NULL;
	u16 index;

	dev_info(&ctrl_info->pci_dev->dev, "pqi_submit_check_inquiry: queue_group %p path %d\n",
		queue_group, path);

	down(&ctrl_info->sync_request_sem);

	atomic_inc(&ctrl_info->num_busy_threads);

	if (!ctrl_info->controller_online) {
		rc = -ENXIO;
		goto out;
	}

	buffer = kmalloc(64, GFP_KERNEL);
	if (!buffer) {
		rc = -ENOMEM;
		goto out;
	}

	index = trustrlib_get_and_update_io_request(ctrl_info, NULL);
	io_request = &ctrl_info->io_request_pool[index];
	request= io_request->iu;

	if (path == RAID_PATH) {
		raid_request = io_request->iu;
		if (pqi_build_inquiry_raid_path_request(ctrl_info, raid_request, RAID_CTLR_LUNID, buffer, 64)) {
			rc = -ENOMEM;
			goto out;
		}
	} else {
		aio_request = io_request->iu;
		if (pqi_build_inquiry_aio_path_request(ctrl_info, aio_request, RAID_CTLR_LUNID, buffer, 64)) {
			rc = -ENOMEM;
			goto out;
		}
	}

	if (path == RAID_PATH) {
		put_unaligned_le16(io_request->index, &(((struct pqi_raid_path_request *)request)->request_id));
		put_unaligned_le16(io_request->index & PQI_IO_REQUEST_INDEX_MASK,
			&(((struct pqi_raid_path_request *)request)->error_index));
	} else {
		put_unaligned_le16(io_request->index, &(((struct pqi_aio_path_request *)request)->request_id));
		put_unaligned_le16(io_request->index & PQI_IO_REQUEST_INDEX_MASK,
			&(((struct pqi_aio_path_request *)request)->error_index));
	}

	wait = kmalloc(sizeof(*wait), GFP_KERNEL);
	if (!wait) {
		rc = -ENOMEM;
		goto unmap;
	}
	init_completion(wait);

	io_request->io_complete_callback = pqi_check_inquiry_complete;
	io_request->context = wait;

	dev_info(&ctrl_info->pci_dev->dev,
		"pqi_submit_check_inquiry: start io_request %p index %x io_complete_callback %p context %p\n",
		io_request, io_request->index, io_request->io_complete_callback, io_request->context);

	pqi_cut_in_line(ctrl_info, queue_group, path, io_request);

	if (wait_for_completion_io_timeout(wait,
			TRUSTRLIB_WAIT_FOR_COMPLETION_IO_TIMEOUT_SECS * HZ) == 0) {
		rc = -ETIMEDOUT;
		goto unmap;
	}

	if (rc == 0 && io_request->error_info) {
		rc = __pqi_process_raid_io_error_synchronous(io_request->error_info);
	}

unmap:
	if (path == RAID_PATH)
		__pqi_pci_unmap(ctrl_info->pci_dev, raid_request->sg_descriptors, 1, DMA_FROM_DEVICE);
	else
		__pqi_pci_unmap(ctrl_info->pci_dev, aio_request->sg_descriptors, 1, DMA_FROM_DEVICE);

out:
	if (wait)
		kfree(wait);
	if (io_request)
		trustrlib_free_io_request(io_request);
	if (buffer)
		kfree(buffer);
	atomic_dec(&ctrl_info->num_busy_threads);
	up(&ctrl_info->sync_request_sem);

	return rc;
}

static void pqi_check_iq_hanging_common(struct pqi_ctrl_info *ctrl_info,
	struct pqi_queue_group *queue_group)
{
	int rc;
	struct trustrlib_queue_group *t = (struct trustrlib_queue_group *)queue_group->trustrlib_data;

	/* submit inquiry to queue, we only care whether it can be executed, not whether the result are correct */
	rc = pqi_submit_check_inquiry(ctrl_info, queue_group, RAID_PATH);
	if (rc) {
		if (rc == -ETIMEDOUT) {
			dev_err(&ctrl_info->pci_dev->dev,
				"queue_group %p RAID_PATH iq hanging\n",
				queue_group);
			t->iq_hanging[RAID_PATH] = 1;
		} else {
			dev_err(&ctrl_info->pci_dev->dev,
				"submit check inquiry error: %d, queue_group %p RAID_PATH\n",
				rc, queue_group);
		}
		return;
	}

	if (t->iq_hanging[RAID_PATH])
		t->iq_hanging[RAID_PATH] = 0;

	rc = pqi_submit_check_inquiry(ctrl_info, queue_group, AIO_PATH);
	if (rc) {
		if (rc == -ETIMEDOUT) {
			dev_err(&ctrl_info->pci_dev->dev,
				"queue_group %p AIO_PATH iq hanging\n",
				queue_group);
			t->iq_hanging[AIO_PATH] = 1;
		} else {
			dev_err(&ctrl_info->pci_dev->dev,
				"submit check inquiry error: %d, queue_group %p AIO_PATH\n",
				rc, queue_group);
		}
		return;
	}

	if (t->iq_hanging[AIO_PATH])
		t->iq_hanging[AIO_PATH] = 0;

	dev_info(&ctrl_info->pci_dev->dev,
		"submit check inquiry complete, queue_group %p\n",
		queue_group);
}

/* handle iq hanging issue. submit inquiry to the queue to trigger the comsumption of its queue elements */
static void pqi_check_iq_hanging_worker(struct work_struct *work)
{
	struct pqi_queue_group *queue_group;
	struct trustrlib_ctrl_info *t;

	t = container_of(work, struct trustrlib_ctrl_info, check_iq_hang_work);
	if (!t->check_queue_group) {
		dev_err(&t->ctrl_info->pci_dev->dev, "check_queue_group is NULL\n");
		return;
	}
	queue_group = (struct pqi_queue_group *)xchg(&t->check_queue_group, NULL);
	dev_info(&t->ctrl_info->pci_dev->dev, "pqi_check_iq_hanging_worker: check_queue_group %p\n",
		queue_group);

	pqi_check_iq_hanging_common(t->ctrl_info, queue_group);
}

/* restart check iq hanging work, to clear iq_hanging status */
static void pqi_recheck_iq_hanging_worker(struct work_struct *work)
{
	struct pqi_queue_group *queue_group;
	struct trustrlib_ctrl_info *t;

	t = container_of(to_delayed_work(work), struct trustrlib_ctrl_info, recheck_iq_hang_work);
	if (!t->recheck_queue_group) {
		dev_err(&t->ctrl_info->pci_dev->dev, "recheck_queue_group is NULL\n");
		return;
	}
	queue_group = (struct pqi_queue_group *)xchg(&t->recheck_queue_group, NULL);
	dev_info(&t->ctrl_info->pci_dev->dev, "pqi_recheck_iq_hanging_worker: recheck_queue_group %p\n",
		queue_group);

	pqi_check_iq_hanging_common(t->ctrl_info, queue_group);
}

static bool trustrlib_match_id(struct pci_device_id *id)
{
	if ((id->subvendor == PCI_VENDOR_ID_HRDT) ||
		(id->subvendor == PCI_VENDOR_ID_POWERLEADER && id->subdevice == 0x104))
		return true;
	return false;
}

int trustrlib_init_ctrl(struct trustrlib_template *template, void *ctrl_info, void *device_id)
{
	struct trustrlib_ctrl_info *t;
	struct pqi_ctrl_info *c = (struct pqi_ctrl_info *)ctrl_info;

	if (!trustrlib_match_id((struct pci_device_id *)device_id)) {
		dev_info(&c->pci_dev->dev, "Unsupport device.\n");
		return -EPERM;
	}

	t = kzalloc(sizeof(struct trustrlib_ctrl_info), GFP_KERNEL);
	if (!t)
		return -ENOMEM;

	t->ctrl_info = c;
	t->ctrl_info->trustrlib_data = t;
	t->template = template;
	t->check_queue_group = NULL;
	t->recheck_queue_group = NULL;

	timer_setup(&t->poll_complete_queue_timer, pqi_poll_complete_queue_timer_handler, 0);
	INIT_WORK(&t->check_iq_hang_work, pqi_check_iq_hanging_worker);
	INIT_DELAYED_WORK(&t->recheck_iq_hang_work, pqi_recheck_iq_hanging_worker);
	return 0;
}

void trustrlib_uninit_ctrl(void *ctrl_info)
{
	struct pqi_ctrl_info *p = (struct pqi_ctrl_info *)ctrl_info;

	kfree(p->trustrlib_data);
	p->trustrlib_data = NULL;
}

void trustrlib_init_works(void *ctrl_info)
{
	struct pqi_ctrl_info *p = (struct pqi_ctrl_info *)ctrl_info;
	struct trustrlib_ctrl_info *t = p->trustrlib_data;

	if (trustrlib_disable_poll_complete_queue)
		return;
	t->poll_complete_queue_timer.expires = jiffies + PQI_POLL_COMPLETE_QUEUE_TIMER_INTERVAL;
	add_timer(&t->poll_complete_queue_timer);
}

void trustrlib_uninit_works(void *ctrl_info)
{
	struct pqi_ctrl_info *p = (struct pqi_ctrl_info *)ctrl_info;
	struct trustrlib_ctrl_info *t = p->trustrlib_data;

	if (trustrlib_disable_poll_complete_queue)
		return;
	del_timer_sync(&t->poll_complete_queue_timer);
	cancel_delayed_work_sync(&t->recheck_iq_hang_work);
}

int trustrlib_init_io_request(void *io_request)
{
	struct pqi_io_request *p = (struct pqi_io_request *)io_request;
	struct trustrlib_io_request *t;

	t = kzalloc(sizeof(struct trustrlib_io_request), GFP_KERNEL);
	if (!t)
		return -ENOMEM;

	spin_lock_init(&t->lock);
	p->trustrlib_data = t;
	return 0;
}

void trustrlib_uninit_io_request(void *io_request)
{
	struct pqi_io_request *p = (struct pqi_io_request *)io_request;

	if (p->trustrlib_data)
		kfree(p->trustrlib_data);
}

int trustrlib_init_queue_group(void *queue_group)
{
	struct pqi_queue_group *p = (struct pqi_queue_group *)queue_group;
	struct trustrlib_queue_group *t;

	t = kzalloc(sizeof(struct trustrlib_queue_group), GFP_KERNEL);
	if (!t)
		return -ENOMEM;

	spin_lock_init(&t->complete_lock);
	p->trustrlib_data = t;
	return 0;
}

void trustrlib_uninit_queue_group(void *queue_group)
{
	struct pqi_queue_group *p = (struct pqi_queue_group *)queue_group;

	if (p->trustrlib_data)
		kfree(p->trustrlib_data);
}

void trustrlib_set_aio_request_id(void *p1, void *p2)
{
	struct pqi_io_request *io_request = (struct pqi_io_request *)p1;
	struct pqi_aio_path_request *request = (struct pqi_aio_path_request *)p2;

	put_unaligned_le16(io_request->index, &request->request_id);
	put_unaligned_le16(io_request->index & PQI_IO_REQUEST_INDEX_MASK, &request->error_index);
}

void trustrlib_set_aio_r1_request_id(void *p1, void *p2)
{
	struct pqi_io_request *io_request = (struct pqi_io_request *)p1;
	struct pqi_aio_r1_path_request *request = (struct pqi_aio_r1_path_request *)p2;

	put_unaligned_le16(io_request->index, &request->request_id);
	put_unaligned_le16(io_request->index & PQI_IO_REQUEST_INDEX_MASK, &request->error_index);
}

void trustrlib_set_aio_r56_request_id(void *p1, void *p2)
{
	struct pqi_io_request *io_request = (struct pqi_io_request *)p1;
	struct pqi_aio_r56_path_request *request = (struct pqi_aio_r56_path_request *)p2;

	put_unaligned_le16(io_request->index, &request->request_id);
	put_unaligned_le16(io_request->index & PQI_IO_REQUEST_INDEX_MASK, &request->error_index);
}

void trustrlib_set_raid_request_id(void *p1, void *p2)
{
	struct pqi_io_request *io_request = (struct pqi_io_request *)p1;
	struct pqi_raid_path_request *request = (struct pqi_raid_path_request *)p2;

	put_unaligned_le16(io_request->index, &request->request_id);
	put_unaligned_le16(io_request->index & PQI_IO_REQUEST_INDEX_MASK, &request->error_index);
}

void trustrlib_reset_io_request_index(void *p)
{
	struct pqi_io_request *io_request = (struct pqi_io_request *)p;
	io_request->index &= PQI_IO_REQUEST_INDEX_MASK;
}

u16 trustrlib_get_and_update_io_request(void *p1, void *p2)
{
	struct pqi_ctrl_info *ctrl_info = (struct pqi_ctrl_info *)p1;
	struct scsi_cmnd *scmd = (struct scsi_cmnd *)p2;
	struct pqi_io_request *io_request;
	struct trustrlib_io_request *t;
	unsigned long flags;
	u16 i;

	if (scmd) {
		u32 blk_tag = blk_mq_unique_tag(PQI_SCSI_REQUEST(scmd));

		i = blk_mq_unique_tag_to_tag(blk_tag);
		if (i < 0 || i >= ctrl_info->scsi_ml_can_queue)
			return ((u16)~0);

		io_request = &ctrl_info->io_request_pool[i];
		t = (struct trustrlib_io_request *)io_request->trustrlib_data;
		spin_lock_irqsave(&t->lock, flags);
		if (atomic_inc_return(&io_request->refcount) > 1) {
			atomic_dec(&io_request->refcount);
			spin_unlock_irqrestore(&t->lock, flags);
			return ((u16)~0);
		}
		PQI_IO_REQUEST_INDEX_UPDATE_MAGIC(io_request->index);
		spin_unlock_irqrestore(&t->lock, flags);
	} else {
		/*
		 * benignly racy - may have to wait for an open slot.
		 */
		i = 0;
		while (1) {
			io_request = &ctrl_info->io_request_pool[ctrl_info->scsi_ml_can_queue + i];
			t = (struct trustrlib_io_request *)io_request->trustrlib_data;
			spin_lock_irqsave(&t->lock, flags);
			if (atomic_inc_return(&io_request->refcount) == 1) {
				PQI_IO_REQUEST_INDEX_UPDATE_MAGIC(io_request->index);
				spin_unlock_irqrestore(&t->lock, flags);
				break;
			}
			atomic_dec(&io_request->refcount);
			spin_unlock_irqrestore(&t->lock, flags);
			i = (i + 1) % PQI_RESERVED_IO_SLOTS;
		}
		i += ctrl_info->scsi_ml_can_queue;
	}

	return i;
}

void trustrlib_free_io_request(void *p)
{
	struct pqi_io_request *io_request = (struct pqi_io_request *)p;
	struct trustrlib_io_request *t = (struct trustrlib_io_request *)io_request->trustrlib_data;
	unsigned long flags;

	if (atomic_dec_return(&t->in_interrupt_or_timedout) < 0) {
		atomic_inc(&t->in_interrupt_or_timedout);
	}

	spin_lock_irqsave(&t->lock, flags);
	atomic_dec(&io_request->refcount);
	spin_unlock_irqrestore(&t->lock, flags);
}

static struct pqi_io_request * pqi_find_io_request(struct pqi_ctrl_info *ctrl_info,
        struct scsi_cmnd *scmd, bool *in_interrupt)
{
	unsigned int i;
	struct pqi_io_request *io_request;
	struct trustrlib_io_request *t;

	for (i = 0; i < ctrl_info->max_io_slots; i++) {
		io_request = &ctrl_info->io_request_pool[i];
		t = (struct trustrlib_io_request *)io_request->trustrlib_data;
		if (scmd == io_request->scmd) {
			if (trustrlib_debug > 0)
				dev_info(&ctrl_info->pci_dev->dev,
					"found io_request %p: index %x  status %d  scmd %p queue_group %p\n",
					io_request, io_request->index,  io_request->status,
					io_request->scmd, io_request->queue_group);

			if (pqi_io_request_referenced(io_request)) {
				if (atomic_inc_return(&t->in_interrupt_or_timedout) == 1) {
					*in_interrupt = false;
				} else {
					atomic_dec(&t->in_interrupt_or_timedout);
					if (trustrlib_debug > 0) {
						dev_info(&ctrl_info->pci_dev->dev,
							"pqi_find_io_request: io_request %p in interrupt\n", io_request);
					}
					*in_interrupt = true;
				}
				return io_request;
			}

			dev_info(&ctrl_info->pci_dev->dev, "pqi_find_io_request continue\n");
		}
	}

	return NULL;
}

static inline bool entry_on_list(struct list_head *entry)
{
	return !(entry->next == NULL || entry->next == LIST_POISON1 || entry->next == entry);
}

/* 0: BLK_EH_DONE; 1: BLK_EH_RESET_TIMER */
int trustrlib_eh_timed_out(void *p)
{
	struct scsi_cmnd *scmd = (struct scsi_cmnd *)p;
	struct Scsi_Host *shost;
	struct pqi_ctrl_info *ctrl_info;
	struct trustrlib_ctrl_info *trustrlib_ctrl;
	struct pqi_scsi_dev *device;
	u8 scsi_opcode;
	struct pqi_io_request *io_request;
	struct trustrlib_io_request *t;
	bool in_interrupt;

	shost = scmd->device->host;
	ctrl_info = shost_to_hba(shost);
	device = scmd->device->hostdata;
	scsi_opcode = scmd->cmd_len > 0 ? scmd->cmnd[0] : 0xff;
	
	if (trustrlib_debug > 0) 
		dev_err(&ctrl_info->pci_dev->dev,
			"timed out scsi %d:%d:%d:%d for SCSI cmd at %p opcode %x reset timer\n",
			shost->host_no, device->bus, device->target, (int)scmd->device->lun, scmd, scsi_opcode);

	io_request = pqi_find_io_request(ctrl_info, scmd, &in_interrupt);
	if (io_request) {
		if (in_interrupt)
			return 1;
		t = (struct trustrlib_io_request *)io_request->trustrlib_data;
		trustrlib_ctrl = (struct trustrlib_ctrl_info *)ctrl_info->trustrlib_data;

		if (entry_on_list(&io_request->request_list_entry)) {
			atomic_dec(&t->in_interrupt_or_timedout);

			if (trustrlib_debug > 0) {
				dev_info(&ctrl_info->pci_dev->dev,
					"io_request %p: index %x on pending list prev %p next %p\n",
					io_request, io_request->index,
					io_request->request_list_entry.prev, io_request->request_list_entry.next);
			}

			if (t->timedout_count > 5)
				return 0;

			if (t->timedout_count > 1) {
				if (cmpxchg(&trustrlib_ctrl->check_queue_group, NULL, io_request->queue_group) == NULL) {
					dev_info(&ctrl_info->pci_dev->dev,
						"start check iq hanging work on queue %p\n", trustrlib_ctrl->check_queue_group);
					schedule_work(&trustrlib_ctrl->check_iq_hang_work);
				}
			}

			t->timedout_count++;

			return 1;
		}

		if (t->timedout_count > 5) {
			atomic_dec(&t->in_interrupt_or_timedout);
			return 0;
		}
		t->timedout_count++;

		if (t->timedout_count == 1) {
			atomic_dec(&t->in_interrupt_or_timedout);
			return 1;
		}

		scsi_dma_unmap(scmd);
		set_host_byte(scmd, DID_IMM_RETRY);
		trustrlib_ctrl->template->inc_scmd_residual(scmd);

		trustrlib_ctrl->template->scsi_done(scmd);
		atomic_dec(&t->in_interrupt_or_timedout);
		trustrlib_free_io_request(io_request);
		return 1;
	}

	return 0;
}

static void pqi_list_del(struct list_head *entry)
{
	list_del(entry);
	if ((entry->next != LIST_POISON1) && (entry->next != entry)) {
		entry->next = NULL;
		entry->prev = NULL;
	}
}

void trustrlib_start_io(void *p1, void *p2, int path)
{
	struct pqi_ctrl_info *ctrl_info = (struct pqi_ctrl_info *)p1;
	struct pqi_queue_group *queue_group = (struct pqi_queue_group *)p2;
	struct pqi_io_request *io_request, *next;
	void *next_element;
	pqi_index_t iq_pi;
	pqi_index_t iq_ci;
	size_t iu_length;
	unsigned int num_elements_needed;
	unsigned int num_elements_to_end_of_queue;
	size_t copy_count;
	struct pqi_iu_header *request;
	int num_elements_submit = 0;
	struct trustrlib_queue_group *t;

	iq_pi = queue_group->iq_pi_copy[path];

	list_for_each_entry_safe(io_request, next, &queue_group->request_list[path], request_list_entry) {
		request = io_request->iu;

		iu_length = get_unaligned_le16(&request->iu_length) + PQI_REQUEST_HEADER_LENGTH;
		num_elements_needed = DIV_ROUND_UP(iu_length, PQI_OPERATIONAL_IQ_ELEMENT_LENGTH);

		iq_ci = readl(queue_group->iq_ci[path]);

		if (num_elements_needed > __pqi_num_elements_free(iq_pi, iq_ci, ctrl_info->num_elements_per_iq))
			break;

		put_unaligned_le16(queue_group->oq_id, &request->response_queue_id);

		next_element = queue_group->iq_element_array[path] +
			(iq_pi * PQI_OPERATIONAL_IQ_ELEMENT_LENGTH);

		num_elements_to_end_of_queue = ctrl_info->num_elements_per_iq - iq_pi;

		if (num_elements_needed <= num_elements_to_end_of_queue) {
			memcpy(next_element, request, iu_length);
		} else {
			copy_count = num_elements_to_end_of_queue * PQI_OPERATIONAL_IQ_ELEMENT_LENGTH;
			memcpy(next_element, request, copy_count);
			memcpy(queue_group->iq_element_array[path],
				(u8 *)request + copy_count,
				iu_length - copy_count);
		}

		iq_pi = (iq_pi + num_elements_needed) % ctrl_info->num_elements_per_iq;

		pqi_list_del(&io_request->request_list_entry);

		num_elements_submit++;
	}

	if (iq_pi != queue_group->iq_pi_copy[path]) {
		t = (struct trustrlib_queue_group *)queue_group->trustrlib_data;
		atomic_add(num_elements_submit, &t->inflight_num);
		queue_group->iq_pi_copy[path] = iq_pi;
		writel(iq_pi, queue_group->iq_pi[path]);
	}
}

u16 trustrlib_alloc_hw_queue(void *p, u16 hint)
{
	struct pqi_ctrl_info *ctrl_info = (struct pqi_ctrl_info *)p;
	struct trustrlib_ctrl_info *trustrlib_ctrl = (struct trustrlib_ctrl_info *)ctrl_info->trustrlib_data;
	struct pqi_queue_group *queue_group;
	struct trustrlib_queue_group *trustrlib_queue;
	u16 hw_queue = hint;

	while (1) {
		queue_group = &ctrl_info->queue_groups[hw_queue];
		trustrlib_queue = (struct trustrlib_queue_group *)queue_group->trustrlib_data;
		if ((trustrlib_queue->iq_hanging[0] == 0) && (trustrlib_queue->iq_hanging[1] == 0))
			break;

		if (cmpxchg(&trustrlib_ctrl->recheck_queue_group, NULL, queue_group) == NULL) {
			dev_info(&ctrl_info->pci_dev->dev,
				"start recheck iq hanging work on queue %p\n", trustrlib_ctrl->recheck_queue_group);
			schedule_delayed_work(&trustrlib_ctrl->recheck_iq_hang_work, PQI_RECHECK_IQ_HANG_DELAY);
		}

		hw_queue = (hw_queue + 1) % ctrl_info->num_queue_groups;
	}

	return hw_queue;
}

void trustrlib_set_sg_table_size(void *p1, void *p2)
{
	struct Scsi_Host *shost = (struct Scsi_Host *)p1;
	struct pqi_ctrl_info *ctrl_info = (struct pqi_ctrl_info *)p2;
	shost->sg_tablesize = min((unsigned short)32, ctrl_info->sg_tablesize);
}

struct trustrlib_ops trustrlib_operations = {
	.init_ctrl = trustrlib_init_ctrl,
	.uninit_ctrl = trustrlib_uninit_ctrl,
	.init_works = trustrlib_init_works,
	.uninit_works = trustrlib_uninit_works,
	.process_io_intr = trustrlib_process_io_intr,
	.init_io_request = trustrlib_init_io_request,
	.uninit_io_request = trustrlib_uninit_io_request,
	.init_queue_group = trustrlib_init_queue_group,
	.uninit_queue_group = trustrlib_uninit_queue_group,
	.set_aio_request_id = trustrlib_set_aio_request_id,
	.set_aio_r1_request_id = trustrlib_set_aio_r1_request_id,
	.set_aio_r56_request_id = trustrlib_set_aio_r56_request_id,
	.set_raid_request_id = trustrlib_set_raid_request_id,
	.reset_io_request_index = trustrlib_reset_io_request_index,
	.get_and_update_io_request = trustrlib_get_and_update_io_request,
	.free_io_request = trustrlib_free_io_request,
	.start_io = trustrlib_start_io,
	.alloc_hw_queue = trustrlib_alloc_hw_queue,
	.set_sg_table_size = trustrlib_set_sg_table_size,
	.eh_timed_out = trustrlib_eh_timed_out,
};

#ifdef KFEATURE_ENABLE_TRUSTRLIB

#ifdef	CONFIG_LOONGARCH
#define VENDOR_OFFSET	0
#define CPUNAME_OFFSET	9
static char cpu_full_name[32] = "        -        ";

static char *get_loongson_cpu_name(void)
{
	uint64_t *vendor = (void *)(&cpu_full_name[VENDOR_OFFSET]);
	uint64_t *cpuname = (void *)(&cpu_full_name[CPUNAME_OFFSET]);

	*vendor = iocsr_read64(LOONGARCH_IOCSR_VENDOR);
	*cpuname = iocsr_read64(LOONGARCH_IOCSR_CPUNAME);

	return cpu_full_name;
}
#endif

#ifdef	CONFIG_SW64
#ifndef MODEL_MAX
#define MODEL_MAX	8
#endif
static char sunway_model_id[64];

static char *get_sunway_model_id(void)
{
	int i;
	unsigned long val;

	memset(sunway_model_id, 0, sizeof(sunway_model_id));
	for (i = 0; i < MODEL_MAX; i++) {
		val = cpuid(GET_MODEL, i);
		memcpy(sunway_model_id + (i * 8), &val, 8);
	}

	return sunway_model_id;
}
#endif

bool trustrlib_match_cpu(void)
{
	char *cpu_name;
	bool retVal = false;
#ifdef CONFIG_LOONGARCH
	cpu_name = get_loongson_cpu_name();
	if ((strncmp(cpu_name, "Loongson-3C5000", 15) == 0) ||
		(strncmp(cpu_name, "Loongson-3A6000", 15) == 0))
		retVal = true;
#else /* CONFIG_SW64 */
	cpu_name = get_sunway_model_id();
	if (cpu_name && (strncmp(cpu_name, "SW3231", 6) == 0))
		retVal = true;
#endif
	return retVal;
}

#endif