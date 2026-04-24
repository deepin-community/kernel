// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 LeapIO Tech Inc.
 *
 * LeapRAID Storage and RAID Controller driver.
 */

#include <linux/compat.h>
#include <linux/module.h>
#include <linux/miscdevice.h>

#include "leapraid_func.h"

/* ioctl device file */
#define LEAPRAID_DEV_NAME       "leapraid_ctl"

/* ioctl version */
#define LEAPRAID_IOCTL_VERSION  0x07

/* ioctl command */
#define LEAPRAID_ADAPTER_INFO   17
#define LEAPRAID_COMMAND        20
#define LEAPRAID_EVENTQUERY     21
#define LEAPRAID_EVENTREPORT    23

/**
 * struct leapraid_ioctl_header - IOCTL command header
 * @adapter_id : Adapter identifier
 * @port_number: Port identifier
 * @max_data_size: Maximum data size for transfer
 */
struct leapraid_ioctl_header {
	u32 adapter_id;
	u32 port_number;
	u32 max_data_size;
};

/**
 * struct leapraid_ioctl_diag_reset - Diagnostic reset request
 * @hdr: Common IOCTL header
 */
struct leapraid_ioctl_diag_reset {
	struct leapraid_ioctl_header hdr;
};

/**
 * struct leapraid_ioctl_pci_info - PCI device information
 * @u: Union holding PCI bus/device/function information
 * @u.bits.dev: PCI device number
 * @u.bits.func: PCI function number
 * @u.bits.bus: PCI bus number
 * @u.word: Combined representation of PCI BDF
 * @seg_id: PCI segment identifier
 */
struct leapraid_ioctl_pci_info {
	union {
		struct {
			u32 dev:5;
			u32 func:3;
			u32 bus:24;
		} bits;
		u32 word;
	} u;
	u32 seg_id;
};

/**
 * struct leapraid_ioctl_adapter_info - Adapter information for IOCTL
 * @hdr: IOCTL header
 * @adapter_type: Adapter type identifier
 * @port_number: Port number
 * @pci_id: PCI device ID
 * @revision: Revision number
 * @sub_dev: Subsystem device ID
 * @sub_vendor: Subsystem vendor ID
 * @r0: Reserved
 * @fw_ver: Firmware version
 * @bios_ver: BIOS version
 * @driver_ver: Driver version
 * @r1: Reserved
 * @scsi_id: SCSI ID
 * @r2: Reserved
 * @pci_info: PCI information structure
 */
struct leapraid_ioctl_adapter_info {
	struct leapraid_ioctl_header hdr;
	u32 adapter_type;
	u32 port_number;
	u32 pci_id;
	u32 revision;
	u32 sub_dev;
	u32 sub_vendor;
	u32 r0;
	u32 fw_ver;
	u32 bios_ver;
	u8 driver_ver[32];
	u8 r1;
	u8 scsi_id;
	u16 r2;
	struct leapraid_ioctl_pci_info pci_info;
};

/**
 * struct leapraid_ioctl_command - IOCTL command structure
 * @hdr: IOCTL header
 * @timeout: Command timeout
 * @rep_msg_buf_ptr: User pointer to reply message buffer
 * @c2h_buf_ptr: User pointer to card-to-host data buffer
 * @h2c_buf_ptr: User pointer to host-to-card data buffer
 * @sense_data_ptr: User pointer to sense data buffer
 * @max_rep_bytes: Maximum reply bytes
 * @c2h_size: Card-to-host data size
 * @h2c_size: Host-to-card data size
 * @max_sense_bytes: Maximum sense data bytes
 * @data_sge_offset: Data SGE offset
 * @mf: Message frame data (flexible array)
 */
struct leapraid_ioctl_command {
	struct leapraid_ioctl_header hdr;
	u32 timeout;
	void __user *rep_msg_buf_ptr;
	void __user *c2h_buf_ptr;
	void __user *h2c_buf_ptr;
	void __user *sense_data_ptr;
	u32 max_rep_bytes;
	u32 c2h_size;
	u32 h2c_size;
	u32 max_sense_bytes;
	u32 data_sge_offset;
	u8 mf[];
};

static struct leapraid_adapter *leapraid_ctl_lookup_adapter(int adapter_id)
{
	struct leapraid_adapter *adapter;

	spin_lock(&leapraid_adapter_lock);
	list_for_each_entry(adapter, &leapraid_adapter_list, list) {
		if (adapter->adapter_attr.id == adapter_id) {
			spin_unlock(&leapraid_adapter_lock);
			return adapter;
		}
	}
	spin_unlock(&leapraid_adapter_lock);

	return NULL;
}

static void leapraid_cli_scsiio_cmd(struct leapraid_adapter *adapter,
				    struct leapraid_req *ctl_sp_mpi_req, u16 taskid,
				    dma_addr_t h2c_dma_addr, size_t h2c_size,
				    dma_addr_t c2h_dma_addr, size_t c2h_size,
				    u16 dev_hdl, void *psge)
{
	struct leapraid_mpi_scsiio_req *scsiio_request =
		(struct leapraid_mpi_scsiio_req *)ctl_sp_mpi_req;

	scsiio_request->sense_buffer_len = SCSI_SENSE_BUFFERSIZE;
	scsiio_request->sense_buffer_low_add =
		 leapraid_get_sense_buffer_dma(adapter, taskid);
	memset((void *)(&adapter->driver_cmds.ctl_cmd.sense),
	       0, SCSI_SENSE_BUFFERSIZE);
	leapraid_build_ieee_sg(adapter, psge, h2c_dma_addr,
			       h2c_size, c2h_dma_addr, c2h_size);
	if (scsiio_request->func == LEAPRAID_FUNC_SCSIIO_REQ)
		leapraid_fire_scsi_io(adapter, taskid, dev_hdl);
	else
		leapraid_fire_task(adapter, taskid);
}

static void leapraid_ctl_smp_passthrough_cmd(struct leapraid_adapter *adapter,
					     struct leapraid_req *ctl_sp_mpi_req,
					     u16 taskid,
					     dma_addr_t h2c_dma_addr,
					     size_t h2c_size,
					     dma_addr_t c2h_dma_addr,
					     size_t c2h_size,
					     void *psge, void *h2c)
{
	struct leapraid_smp_passthrough_req *smp_pt_req =
		 (struct leapraid_smp_passthrough_req *)ctl_sp_mpi_req;
	u8 *data;

	if (!adapter->adapter_attr.enable_mp)
		smp_pt_req->physical_port = LEAPRAID_DISABLE_MP_PORT_ID;
	if (smp_pt_req->passthrough_flg & LEAPRAID_SMP_PT_FLAG_SGL_PTR)
		data = (u8 *)&smp_pt_req->sgl;
	else
		data = h2c;

	if (data[1] == LEAPRAID_SMP_FN_REPORT_PHY_ERR_LOG &&
	    (data[10] == 1 || data[10] == 2))
		adapter->reset_desc.adapter_link_resetting = true;
	leapraid_build_ieee_sg(adapter, psge, h2c_dma_addr,
			       h2c_size, c2h_dma_addr, c2h_size);
	leapraid_fire_task(adapter, taskid);
}

static void leapraid_ctl_fire_ieee_cmd(struct leapraid_adapter *adapter,
				       dma_addr_t h2c_dma_addr,
				       size_t h2c_size,
				       dma_addr_t c2h_dma_addr,
				       size_t c2h_size,
				       void *psge, u16 taskid)
{
	leapraid_build_ieee_sg(adapter, psge, h2c_dma_addr, h2c_size,
			       c2h_dma_addr, c2h_size);
	leapraid_fire_task(adapter, taskid);
}

static void leapraid_ctl_sata_passthrough_cmd(struct leapraid_adapter *adapter,
					      dma_addr_t h2c_dma_addr,
					      size_t h2c_size,
					      dma_addr_t c2h_dma_addr,
					      size_t c2h_size,
					      void *psge, u16 taskid)
{
	leapraid_ctl_fire_ieee_cmd(adapter, h2c_dma_addr,
				   h2c_size, c2h_dma_addr,
				   c2h_size, psge, taskid);
}

static void leapraid_ctl_load_fw_cmd(struct leapraid_adapter *adapter,
				     dma_addr_t h2c_dma_addr, size_t h2c_size,
				     dma_addr_t c2h_dma_addr, size_t c2h_size,
				     void *psge, u16 taskid)
{
	leapraid_ctl_fire_ieee_cmd(adapter, h2c_dma_addr,
				   h2c_size, c2h_dma_addr,
				   c2h_size, psge, taskid);
}

static void leapraid_ctl_fire_mpi_cmd(struct leapraid_adapter *adapter,
				      dma_addr_t h2c_dma_addr, size_t h2c_size,
				      dma_addr_t c2h_dma_addr, size_t c2h_size,
				      void *psge, u16 taskid)
{
	leapraid_build_mpi_sg(adapter, psge, h2c_dma_addr,
			      h2c_size, c2h_dma_addr, c2h_size);
	leapraid_fire_task(adapter, taskid);
}

static void leapraid_ctl_sas_io_unit_ctrl_cmd(struct leapraid_adapter *adapter,
					      struct leapraid_req *ctl_sp_mpi_req,
					      dma_addr_t h2c_dma_addr,
					      size_t h2c_size,
					      dma_addr_t c2h_dma_addr,
					      size_t c2h_size,
					      void *psge, u16 taskid)
{
	struct leapraid_sas_io_unit_ctrl_req *sas_io_unit_ctrl_req =
		(struct leapraid_sas_io_unit_ctrl_req *)ctl_sp_mpi_req;

	if (sas_io_unit_ctrl_req->op == LEAPRAID_SAS_OP_PHY_HARD_RESET ||
	    sas_io_unit_ctrl_req->op == LEAPRAID_SAS_OP_PHY_LINK_RESET)
		adapter->reset_desc.adapter_link_resetting = true;
	leapraid_ctl_fire_mpi_cmd(adapter, h2c_dma_addr,
				  h2c_size, c2h_dma_addr,
				  c2h_size, psge, taskid);
}

static long leapraid_ctl_do_command(struct leapraid_adapter *adapter,
				    struct leapraid_ioctl_command *karg,
				    void __user *mf)
{
	struct leapraid_req *leap_mpi_req = NULL;
	struct leapraid_req *ctl_sp_mpi_req = NULL;
	u16 taskid;
	void *h2c = NULL;
	size_t h2c_size = 0;
	dma_addr_t h2c_dma_addr = 0;
	void *c2h = NULL;
	size_t c2h_size = 0;
	dma_addr_t c2h_dma_addr = 0;
	void *psge;
	unsigned long timeout;
	u16 dev_hdl = LEAPRAID_INVALID_DEV_HANDLE;
	bool issue_reset = false;
	u32 sz;
	long rc = 0;

	rc = leapraid_check_adapter_is_op(adapter);
	if (rc)
		goto out;

	leap_mpi_req = kzalloc(LEAPRAID_REQUEST_SIZE, GFP_KERNEL);
	if (!leap_mpi_req) {
		rc = -ENOMEM;
		goto out;
	}

	if (karg->data_sge_offset * LEAPRAID_SGE_OFFSET_SIZE > LEAPRAID_REQUEST_SIZE ||
	    karg->data_sge_offset > ((UINT_MAX) / LEAPRAID_SGE_OFFSET_SIZE)) {
		rc = -EINVAL;
		goto out;
	}

	if (copy_from_user(leap_mpi_req, mf,
			   karg->data_sge_offset * LEAPRAID_SGE_OFFSET_SIZE)) {
		rc = -EFAULT;
		goto out;
	}

	taskid = adapter->driver_cmds.ctl_cmd.taskid;

	adapter->driver_cmds.ctl_cmd.status = LEAPRAID_CMD_PENDING;
	memset((void *)(&adapter->driver_cmds.ctl_cmd.reply), 0,
	       LEAPRAID_REPLY_SIEZ);
	ctl_sp_mpi_req = leapraid_get_task_desc(adapter, taskid);
	memset(ctl_sp_mpi_req, 0, LEAPRAID_REQUEST_SIZE);
	memcpy(ctl_sp_mpi_req,
	       leap_mpi_req,
	       karg->data_sge_offset * LEAPRAID_SGE_OFFSET_SIZE);

	if (ctl_sp_mpi_req->func == LEAPRAID_FUNC_SCSIIO_REQ ||
	    ctl_sp_mpi_req->func == LEAPRAID_FUNC_RAID_SCSIIO_PASSTHROUGH ||
	    ctl_sp_mpi_req->func == LEAPRAID_FUNC_SATA_PASSTHROUGH) {
		dev_hdl = le16_to_cpu(ctl_sp_mpi_req->func_dep1);
		if (!dev_hdl || dev_hdl > adapter->adapter_attr.features.max_dev_handle) {
			rc = -EINVAL;
			goto out;
		}
	}

	if (WARN_ON(ctl_sp_mpi_req->func == LEAPRAID_FUNC_SCSI_TMF))
		return -EINVAL;

	h2c_size = karg->h2c_size;
	c2h_size = karg->c2h_size;
	if (h2c_size) {
		h2c = dma_alloc_coherent(&adapter->pdev->dev, h2c_size,
					 &h2c_dma_addr, GFP_ATOMIC);
		if (!h2c) {
			rc = -ENOMEM;
			goto out;
		}
		if (copy_from_user(h2c, karg->h2c_buf_ptr, h2c_size)) {
			rc = -EFAULT;
			goto out;
		}
	}
	if (c2h_size) {
		c2h = dma_alloc_coherent(&adapter->pdev->dev,
					 c2h_size, &c2h_dma_addr, GFP_ATOMIC);
		if (!c2h) {
			rc = -ENOMEM;
			goto out;
		}
	}

	psge = (void *)ctl_sp_mpi_req + (karg->data_sge_offset *
					 LEAPRAID_SGE_OFFSET_SIZE);
	init_completion(&adapter->driver_cmds.ctl_cmd.done);

	switch (ctl_sp_mpi_req->func) {
	case LEAPRAID_FUNC_SCSIIO_REQ:
	case LEAPRAID_FUNC_RAID_SCSIIO_PASSTHROUGH:
		if (test_bit(dev_hdl, (unsigned long *)adapter->dev_topo.dev_removing)) {
			rc = -EINVAL;
			goto out;
		}
		leapraid_cli_scsiio_cmd(adapter, ctl_sp_mpi_req, taskid,
					h2c_dma_addr, h2c_size,
					c2h_dma_addr, c2h_size,
					dev_hdl, psge);
		break;
	case LEAPRAID_FUNC_SMP_PASSTHROUGH:
		if (!h2c) {
			rc = -EINVAL;
			goto out;
		}
		leapraid_ctl_smp_passthrough_cmd(adapter,
						 ctl_sp_mpi_req, taskid,
						 h2c_dma_addr, h2c_size,
						 c2h_dma_addr, c2h_size,
						 psge, h2c);
		break;
	case LEAPRAID_FUNC_SATA_PASSTHROUGH:
		if (test_bit(dev_hdl, (unsigned long *)adapter->dev_topo.dev_removing)) {
			rc = -EINVAL;
			goto out;
		}
		leapraid_ctl_sata_passthrough_cmd(adapter, h2c_dma_addr,
						  h2c_size, c2h_dma_addr,
						  c2h_size, psge, taskid);
		break;
	case LEAPRAID_FUNC_FW_DOWNLOAD:
	case LEAPRAID_FUNC_FW_UPLOAD:
		leapraid_ctl_load_fw_cmd(adapter, h2c_dma_addr,
					 h2c_size, c2h_dma_addr,
					 c2h_size, psge, taskid);
		break;
	case LEAPRAID_FUNC_SAS_IO_UNIT_CTRL:
		leapraid_ctl_sas_io_unit_ctrl_cmd(adapter, ctl_sp_mpi_req,
						  h2c_dma_addr, h2c_size,
						  c2h_dma_addr, c2h_size,
						  psge, taskid);
		break;
	default:
		leapraid_ctl_fire_mpi_cmd(adapter, h2c_dma_addr,
					  h2c_size, c2h_dma_addr,
					  c2h_size, psge, taskid);
		break;
	}

	timeout = karg->timeout;
	if (timeout < LEAPRAID_CTL_CMD_TIMEOUT)
		timeout = LEAPRAID_CTL_CMD_TIMEOUT;
	wait_for_completion_timeout(&adapter->driver_cmds.ctl_cmd.done,
				    timeout * HZ);

	if ((leap_mpi_req->func == LEAPRAID_FUNC_SMP_PASSTHROUGH ||
	     leap_mpi_req->func == LEAPRAID_FUNC_SAS_IO_UNIT_CTRL) &&
	    adapter->reset_desc.adapter_link_resetting) {
		adapter->reset_desc.adapter_link_resetting = false;
	}
	if (!(adapter->driver_cmds.ctl_cmd.status & LEAPRAID_CMD_DONE)) {
		issue_reset =
			leapraid_check_reset(
				adapter->driver_cmds.ctl_cmd.status);
		goto reset;
	}

	if (c2h_size) {
		if (copy_to_user(karg->c2h_buf_ptr, c2h, c2h_size)) {
			rc = -ENODATA;
			goto out;
		}
	}
	if (karg->max_rep_bytes) {
		sz = min_t(u32, karg->max_rep_bytes, LEAPRAID_REPLY_SIEZ);
		if (copy_to_user(karg->rep_msg_buf_ptr,
				 (void *)&adapter->driver_cmds.ctl_cmd.reply,
				 sz)) {
			rc = -ENODATA;
			goto out;
		}
	}

	if (karg->max_sense_bytes &&
	    (leap_mpi_req->func == LEAPRAID_FUNC_SCSIIO_REQ ||
	     leap_mpi_req->func == LEAPRAID_FUNC_RAID_SCSIIO_PASSTHROUGH)) {
		if (!karg->sense_data_ptr)
			goto out;

		sz = min_t(u32, karg->max_sense_bytes, SCSI_SENSE_BUFFERSIZE);
		if (copy_to_user(karg->sense_data_ptr,
				 (void *)&adapter->driver_cmds.ctl_cmd.sense,
				 sz)) {
			rc = -ENODATA;
			goto out;
		}
	}
reset:
	if (issue_reset) {
		rc = -ENODATA;
		if (leap_mpi_req->func == LEAPRAID_FUNC_SCSIIO_REQ ||
		    leap_mpi_req->func == LEAPRAID_FUNC_RAID_SCSIIO_PASSTHROUGH ||
		    leap_mpi_req->func == LEAPRAID_FUNC_SATA_PASSTHROUGH) {
			dev_err(&adapter->pdev->dev,
				"fire tgt reset: hdl=0x%04x\n",
				le16_to_cpu(leap_mpi_req->func_dep1));
			leapraid_issue_locked_tm(adapter,
				le16_to_cpu(leap_mpi_req->func_dep1), 0, 0, 0,
				LEAPRAID_TM_TASKTYPE_TARGET_RESET, taskid,
				LEAPRAID_TM_MSGFLAGS_LINK_RESET);
		} else {
			dev_info(&adapter->pdev->dev,
				 "%s:%d call hard_reset\n",
				 __func__, __LINE__);
			leapraid_hard_reset_handler(adapter, FULL_RESET);
		}
	}
out:
	if (c2h)
		dma_free_coherent(&adapter->pdev->dev, c2h_size,
				  c2h, c2h_dma_addr);
	if (h2c)
		dma_free_coherent(&adapter->pdev->dev, h2c_size,
				  h2c, h2c_dma_addr);
	kfree(leap_mpi_req);
	adapter->driver_cmds.ctl_cmd.status = LEAPRAID_CMD_NOT_USED;
	return rc;
}

static long leapraid_ctl_get_adapter_info(struct leapraid_adapter *adapter,
					  void __user *arg)
{
	struct leapraid_ioctl_adapter_info *karg;
	ssize_t __maybe_unused ret;
	u8 revision;

	karg = kzalloc(sizeof(*karg), GFP_KERNEL);
	if (!karg)
		return -ENOMEM;

	pci_read_config_byte(adapter->pdev, PCI_CLASS_REVISION, &revision);
	karg->revision = revision;
	karg->pci_id = adapter->pdev->device;
	karg->sub_dev = adapter->pdev->subsystem_device;
	karg->sub_vendor = adapter->pdev->subsystem_vendor;
	karg->pci_info.u.bits.bus = adapter->pdev->bus->number;
	karg->pci_info.u.bits.dev = PCI_SLOT(adapter->pdev->devfn);
	karg->pci_info.u.bits.func = PCI_FUNC(adapter->pdev->devfn);
	karg->pci_info.seg_id = pci_domain_nr(adapter->pdev->bus);
	karg->fw_ver = adapter->adapter_attr.features.fw_version;
	ret = strscpy(karg->driver_ver, LEAPRAID_DRIVER_NAME,
		      sizeof(karg->driver_ver));
	strcat(karg->driver_ver, "-");
	strcat(karg->driver_ver, LEAPRAID_DRIVER_VERSION);
	karg->adapter_type = LEAPRAID_IOCTL_VERSION;
	karg->bios_ver = adapter->adapter_attr.bios_version;
	if (copy_to_user(arg, karg,
			 sizeof(struct leapraid_ioctl_adapter_info))) {
		kfree(karg);
		return -EFAULT;
	}

	kfree(karg);
	return 0;
}

static long leapraid_ctl_ioctl_main(struct file *file, unsigned int cmd,
				    void __user *arg, u8 compat)
{
	struct leapraid_ioctl_header ioctl_header;
	struct leapraid_adapter *adapter;
	long rc = -ENOIOCTLCMD;
	int count;

	if (copy_from_user(&ioctl_header, (char __user *)arg,
			   sizeof(struct leapraid_ioctl_header)))
		return -EFAULT;

	adapter = leapraid_ctl_lookup_adapter(ioctl_header.adapter_id);
	if (!adapter)
		return -EFAULT;

	mutex_lock(&adapter->access_ctrl.pci_access_lock);

	rc = leapraid_check_adapter_is_op(adapter);
	if (rc)
		goto out;

	count = LEAPRAID_WAIT_SHOST_RECOVERY;
	while (count--) {
		if (!adapter->access_ctrl.shost_recovering)
			break;
		ssleep(1);
	}

	if (adapter->access_ctrl.shost_recovering ||
	    adapter->access_ctrl.pcie_recovering ||
	    adapter->scan_dev_desc.driver_loading ||
	    adapter->access_ctrl.host_removing) {
		rc = -EAGAIN;
		goto out;
	}

	if (file->f_flags & O_NONBLOCK) {
		if (!mutex_trylock(&adapter->driver_cmds.ctl_cmd.mutex)) {
			rc = -EAGAIN;
			goto out;
		}
	} else if (mutex_lock_interruptible(&adapter->driver_cmds.ctl_cmd.mutex)) {
		rc = -ERESTARTSYS;
		goto out;
	}

	switch (_IOC_NR(cmd)) {
	case LEAPRAID_ADAPTER_INFO:
		if (_IOC_SIZE(cmd) == sizeof(struct leapraid_ioctl_adapter_info))
			rc = leapraid_ctl_get_adapter_info(adapter, arg);
		break;
	case LEAPRAID_COMMAND:
	{
		struct leapraid_ioctl_command __user *uarg;
		struct leapraid_ioctl_command karg;

		if (copy_from_user(&karg, arg, sizeof(karg))) {
			rc = -EFAULT;
			break;
		}

		if (karg.hdr.adapter_id  != ioctl_header.adapter_id) {
			rc = -EINVAL;
			break;
		}

		if (_IOC_SIZE(cmd) == sizeof(struct leapraid_ioctl_command)) {
			uarg = arg;
			rc = leapraid_ctl_do_command(adapter, &karg,
						     &uarg->mf);
		}
		break;
	}
	case LEAPRAID_EVENTQUERY:
	case LEAPRAID_EVENTREPORT:
		rc = 0;
		break;
	default:
		pr_err("unknown ioctl opcode=0x%08x\n", cmd);
		break;
	}
	mutex_unlock(&adapter->driver_cmds.ctl_cmd.mutex);

out:
	mutex_unlock(&adapter->access_ctrl.pci_access_lock);
	return rc;
}

static long leapraid_ctl_ioctl(struct file *file, unsigned int cmd,
			       unsigned long arg)
{
	return leapraid_ctl_ioctl_main(file, cmd,
				       (void __user *)arg, 0);
}

static int leapraid_fw_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct leapraid_adapter *adapter;
	unsigned long length;
	unsigned long pfn;

	length = vma->vm_end - vma->vm_start;

	adapter = list_first_entry(&leapraid_adapter_list,
				   struct leapraid_adapter, list);

	if (length > (LEAPRAID_SYS_LOG_BUF_SIZE +
		      LEAPRAID_SYS_LOG_BUF_RESERVE)) {
		dev_err(&adapter->pdev->dev,
			"requested mapping size is too large!\n");
		return -EINVAL;
	}

	if (!adapter->fw_log_desc.fw_log_buffer) {
		dev_err(&adapter->pdev->dev, "no log buffer!\n");
		return -EINVAL;
	}

	pfn = virt_to_phys(adapter->fw_log_desc.fw_log_buffer) >> PAGE_SHIFT;

	if (remap_pfn_range(vma, vma->vm_start, pfn, length,
			    vma->vm_page_prot)) {
		dev_err(&adapter->pdev->dev,
			"failed to map memory to user space!\n");
		return -EAGAIN;
	}

	return 0;
}

static const struct file_operations leapraid_ctl_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = leapraid_ctl_ioctl,
	.mmap = leapraid_fw_mmap,
};

static struct miscdevice leapraid_ctl_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = LEAPRAID_DEV_NAME,
	.fops = &leapraid_ctl_fops,
};

void leapraid_ctl_init(void)
{
	if (misc_register(&leapraid_ctl_dev) < 0)
		pr_err("%s can't register misc device\n", LEAPRAID_DRIVER_NAME);
}

void leapraid_ctl_exit(void)
{
	misc_deregister(&leapraid_ctl_dev);
}
