#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/uio.h>
#include <linux/irq_poll.h>
#include <scsi/scsi_transport.h>
#include <scsi/scsi_host.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/aer.h>
#include <scsi/scsi_tcq.h>

#include "ps3_htp_def.h"
#include "ps3_htp_event.h"
#include "ps3_htp_ioctl.h"
#include "ps3_cli.h"
#include "ps3_instance_manager.h"
#include "ps3_ioc_manager.h"
#include "ps3_ioc_state.h"
#include "ps3_scsih.h"
#include "ps3_driver_log.h"
#include "ps3_cmd_channel.h"
#include "ps3_mgr_cmd.h"
#include "ps3_event.h"
#include "ps3_mgr_cmd_err.h"
#include "ps3_scsi_cmd_err.h"
#include "ps3_device_manager.h"
#include "ps3_cmd_complete.h"
#include "ps3_device_update.h"
#include "ps3_debug.h"
#include "ps3_ioctl.h"
#include "ps3_driver_log.h"
#include "ps3_inner_data.h"
#include "ps3_cmd_statistics.h"
#include "ps3_trace_id_alloc.h"
#include "ps3_module_para.h"
#include "ps3_scsi_cmd_err.h"
#include "ps3_drv_ver.h"
#include "ps3_pcie_err_handle.h"
#include "ps3_err_inject.h"
#include "ps3_r1x_write_lock.h"
#include "ps3_watchdog.h"
#if	defined PS3_HARDWARE_ASIC
#include "ps3_pci.h"
#endif
#include "ps3_cli_debug.h"

const char *const PS3_CHRDEV_NAME = "ps3stor-ioctl";
#define PS3_PCI_DRIVER_NAME  "ps3stor"
#define PS3_SCSI_HOST_NAME  "ps3stor_scsi_host"
#define PS3_SCSI_DEFAULT_INIT_ID (-1)
#define PS3_SCSI_DEFAULT_CMD_PER_LUN (256)

#define PS3_BIOS_PARAM_DEFAULT_HEADER (64)
#define PS3_BIOS_PARAM_DEFAULT_SECTORS (32)
#define PS3_BIOS_PARAM_MAX_HEADER (255)
#define PS3_BIOS_PARAM_MAX_SECTORS (63)
#define PS3_BIOS_PARAM_DEV_CAPCITY_THRESHOLD_1G (0x200000)

#define IS_DMA64	(sizeof(dma_addr_t) == 8)
#define PS3_HOST_UUIID(_dev) (_dev->bus->number << 8 | _dev->devfn)
static const U8 DMA_MASK_BIT_44 = PCIE_DMA_HOST_ADDR_BIT_POS;
static const U8 DMA_MASK_BIT_52 = PCIE_DMA_HOST_ADDR_BIT_POS_VALID;
static const U8 DMA_MASK_BIT_32 = 32;
static const U32 PS3_REGISTER_BAR_INDEX = 0x02;

static unsigned int ps3_mgmt_chrdev_major_no;

#ifdef __KERNEL__
#define PCI_DEVICE_SET(vend, dev, subven, subdev, cls, cls_mask) \
	.vendor = (vend), .device = (dev), \
	.subvendor = (subven), .subdevice = (subdev),\
	.class = (cls), .class_mask = (cls_mask)
#else
#define PCI_DEVICE_SET(vend, dev, subven, subdev, class, class_mask) \
	.vendor = (vend), .device = (dev), \
	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID
#endif

static struct pci_device_id ps3_pci_table[] = {
	{PCI_DEVICE(PCI_VENDOR_ID_PS3_FPGA, PCI_DEVICE_ID_PS3_RAID_FPGA)},
	{PCI_DEVICE(PCI_VENDOR_ID_PS3_FPGA, PCI_DEVICE_ID_PS3_HBA)},
	{PCI_DEVICE(PCI_VENDOR_ID_PS3_ASIC, PCI_DEVICE_ID_PS3_RAID_FPGA)},
	{PCI_DEVICE(PCI_VENDOR_ID_PS3_ASIC, PCI_DEVICE_ID_PS3_HBA)},
	{PCI_DEVICE(PCI_VENDOR_ID_STARS, PCI_DEVICE_ID_STARS_IOC_2020_18i)},
	{PCI_DEVICE(PCI_VENDOR_ID_STARS, PCI_DEVICE_ID_STARS_ROC_2020_10i)},
	{PCI_DEVICE(PCI_VENDOR_ID_PS3, PCI_DEVICE_ID_STARS_IOC_2020_18i)},
	{PCI_DEVICE(PCI_VENDOR_ID_PS3, PCI_DEVICE_ID_STARS_ROC_2020_10i)},
	{PCI_DEVICE(PCI_VENDOR_ID_PS3, PCI_DEVICE_ID_STARS_IOC_2213_16i)},
	{0, 0, 0, 0}};

MODULE_DEVICE_TABLE(pci, ps3_pci_table);

static int ps3_mgmt_open(struct inode *pinode, struct file *filep)
{
	(void)pinode;
	(void)filep;
	if (!capable(CAP_SYS_ADMIN)) {
		return -EACCES;
	}

	return PS3_SUCCESS;
}

static int ps3_bios_param(struct scsi_device *sdev, struct block_device *bdev,
	sector_t capacity, int params[])
{
	int heads = PS3_BIOS_PARAM_DEFAULT_HEADER;
	int sectors = PS3_BIOS_PARAM_DEFAULT_SECTORS;
	sector_t cylinders = capacity;
	ULong dummy = heads * sectors;
	(void)sdev;
	(void)bdev;

	sector_div(cylinders, dummy);

	if ((ULong)capacity >= PS3_BIOS_PARAM_DEV_CAPCITY_THRESHOLD_1G) {
		heads = PS3_BIOS_PARAM_MAX_HEADER;
		sectors = PS3_BIOS_PARAM_MAX_SECTORS;
		dummy = heads * sectors;
		cylinders = capacity;
		sector_div(cylinders, dummy);
	}

	params[0] = heads;
	params[1] = sectors;
	params[2] = cylinders;

	return 0;
}

#if defined(PS3_TAGSET_SUPPORT)
static MAP_QUEUES_RET_TYPE ps3_map_queues(struct Scsi_Host *shost)
{
	struct ps3_instance *instance = NULL;
	S32 qoff = 0;
	S32 offset = 0;
	struct blk_mq_queue_map *map = NULL;

	instance = (struct ps3_instance *)shost->hostdata;

	if (shost->nr_hw_queues == 1) {
		goto l_out;
	}

	offset = instance->irq_context.high_iops_msix_vectors;
	qoff = 0;

	map = &shost->tag_set.map[HCTX_TYPE_DEFAULT];
	map->nr_queues = instance->irq_context.valid_msix_vector_count - offset;
	map->queue_offset = 0;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,12,0))
	(void)qoff;
	MAP_QUEUES_RET_VAL(blk_mq_pci_map_queues(map, instance->pdev, offset));
#else
	blk_mq_pci_map_queues(map, instance->pdev, offset);
	qoff += map->nr_queues;

	map = &shost->tag_set.map[HCTX_TYPE_POLL];
	map->nr_queues = 0;
	if (map->nr_queues) {
		map->queue_offset = qoff;
		blk_mq_map_queues(map);
	}
#endif
l_out:
	MAP_QUEUES_RET_VAL(0);
}

#endif

static ssize_t page_size_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	if (buf == NULL) {
		ret = 0;
		goto l_out;
	}
	(void)cdev;
	(void)attr;

 	ret = snprintf(buf, PAGE_SIZE, "%ld\n", (ULong)PAGE_SIZE - 1);

l_out:
	return ret;
}

static inline ssize_t vd_io_outstanding_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_vd_io_outstanding_show(cdev, attr, buf);
}

static inline ssize_t io_outstanding_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_io_outstanding_show(cdev, attr, buf);
}

static inline ssize_t is_load_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_is_load_show(cdev, attr, buf);
}

static inline ssize_t dump_ioc_regs_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_dump_ioc_regs_show(cdev, attr, buf);
}

static inline ssize_t max_scsi_cmds_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_max_scsi_cmds_show(cdev, attr, buf);
}

static inline ssize_t event_subscribe_info_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_event_subscribe_info_show(cdev, attr, buf);
}

static inline ssize_t ioc_state_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_ioc_state_show(cdev, attr, buf);
}

static ssize_t log_level_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return ps3_log_level_store(cdev, attr, buf, count);
}

static inline ssize_t log_level_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_log_level_show(cdev, attr, buf);
}

static inline ssize_t io_trace_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return ps3_io_trace_switch_store(cdev, attr, buf, count);
}

static inline ssize_t io_trace_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_io_trace_switch_show(cdev, attr, buf);
}

static inline ssize_t dump_type_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_dump_type_show(cdev, attr, buf);
}

static inline ssize_t dump_type_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return ps3_dump_type_store(cdev, attr, buf, count);
}

static inline ssize_t dump_dir_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_dump_dir_show(cdev, attr, buf);
}

static inline ssize_t dump_state_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_dump_state_show(cdev, attr, buf);
}

static inline ssize_t dump_state_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return ps3_dump_state_store(cdev, attr, buf, count);
}

static inline ssize_t soc_dead_reset_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return ps3_soc_dead_reset_store(cdev, attr, buf, count);
}
static inline ssize_t halt_support_cli_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_halt_support_cli_show(cdev, attr, buf);
}

static inline ssize_t halt_support_cli_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return ps3_halt_support_cli_store(cdev, attr, buf, count);
}

static inline ssize_t qos_switch_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_qos_switch_show(cdev, attr, buf);
}

static ssize_t qos_switch_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return ps3_qos_switch_store(cdev, attr, buf, count);
}
static inline ssize_t product_model_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_product_model_show(cdev, attr, buf);
}

#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
#else

static inline ssize_t irq_prk_support_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	return ps3_irq_prk_support_show(cdev, attr, buf);
}

static inline ssize_t irq_prk_support_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return ps3_irq_prk_support_store(cdev, attr, buf, count);
}
#endif

static inline ssize_t cli_ver_show(struct device *cdev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret= 0;
	(void)cdev;
	(void)attr;
	if (buf != NULL) {
		ret = snprintf(buf, PAGE_SIZE, "%d\n", PS3_IOCTL_VERSION);
	}

	return ret;
}

static DEVICE_ATTR_RO(page_size);
static DEVICE_ATTR_RO(vd_io_outstanding);
static DEVICE_ATTR_RO(io_outstanding);
static DEVICE_ATTR_RO(is_load);
static DEVICE_ATTR_RO(dump_ioc_regs);
static DEVICE_ATTR_RO(max_scsi_cmds);
static DEVICE_ATTR_RO(event_subscribe_info);
static DEVICE_ATTR_RO(ioc_state);
static DEVICE_ATTR_RW(log_level);
static DEVICE_ATTR_RW(io_trace);
static DEVICE_ATTR_RO(dump_dir);
static DEVICE_ATTR_RW(dump_type);
static DEVICE_ATTR_RW(dump_state);
static DEVICE_ATTR_WO(soc_dead_reset);
static DEVICE_ATTR_RW(halt_support_cli);
static DEVICE_ATTR_RW(qos_switch);
static DEVICE_ATTR_RO(product_model);
static DEVICE_ATTR_RO(cli_ver);

#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
#else
static DEVICE_ATTR_RW(irq_prk_support);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0))
static struct attribute *ps3_host_attrs[] = {
	&dev_attr_page_size.attr,
	&dev_attr_vd_io_outstanding.attr,
	&dev_attr_io_outstanding.attr,
	&dev_attr_is_load.attr,
	&dev_attr_dump_ioc_regs.attr,
	&dev_attr_max_scsi_cmds.attr,
	&dev_attr_event_subscribe_info.attr,
	&dev_attr_ioc_state.attr,
	&dev_attr_log_level.attr,
	&dev_attr_io_trace.attr,
	&dev_attr_dump_state.attr,
	&dev_attr_dump_type.attr,
	&dev_attr_dump_dir.attr,
	&dev_attr_soc_dead_reset.attr,
	&dev_attr_halt_support_cli.attr,
	&dev_attr_qos_switch.attr,
	&dev_attr_product_model.attr,
	&dev_attr_cli_ver.attr,
#if defined(PS3_SUPPORT_DEBUG) || \
		(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
		(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
#else
	&dev_attr_irq_prk_support.attr,
#endif
	NULL,
};

static const struct attribute_group ps3_host_attr_group = {
	.attrs = ps3_host_attrs
};

const struct attribute_group *ps3_host_groups[] = {
	&ps3_host_attr_group,
	NULL
};
#else
static struct device_attribute *ps3_host_attrs[] = {
	&dev_attr_page_size,
	&dev_attr_vd_io_outstanding,
	&dev_attr_io_outstanding,
	&dev_attr_is_load,
	&dev_attr_dump_ioc_regs,
	&dev_attr_max_scsi_cmds,
	&dev_attr_event_subscribe_info,
	&dev_attr_ioc_state,
	&dev_attr_log_level,
	&dev_attr_io_trace,
	&dev_attr_dump_state,
	&dev_attr_dump_type,
	&dev_attr_dump_dir,
	&dev_attr_soc_dead_reset,
	&dev_attr_halt_support_cli,
	&dev_attr_qos_switch,
	&dev_attr_product_model,
	&dev_attr_cli_ver,
#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
#else
	&dev_attr_irq_prk_support,
#endif
	NULL,
};
#endif
static struct scsi_host_template ps3_scsi_host_template = {
	.module = THIS_MODULE,
	.name = PS3_SCSI_HOST_NAME,
	.proc_name = PS3_SCSI_HOST_PROC_NAME,
	.queuecommand = ps3_scsih_queue_command,
	.eh_abort_handler = ps3_err_scsi_task_mgr_abort,
	.slave_alloc = ps3_scsi_slave_alloc,
	.slave_configure = ps3_scsi_slave_configure,
	.slave_destroy = ps3_scsi_slave_destroy,
	.change_queue_depth = ps3_change_queue_depth,
	.eh_device_reset_handler = ps3_device_reset_handler,
	.eh_target_reset_handler = ps3_err_reset_target,
	.eh_host_reset_handler = ps3_err_reset_host,
	.eh_timed_out = ps3_err_reset_timer,
	.bios_param = ps3_bios_param,
#if defined(PS3_TAGSET_SUPPORT)
	.map_queues = ps3_map_queues,
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0))
	.shost_groups = ps3_host_groups,
#else
	.shost_attrs = ps3_host_attrs,
#endif
};

static inline void ps3_set_product_model(struct ps3_instance * instance, const U16 id)
{
	if (PS3_DEVICE_IS_SWITCH(id)) {
		instance->product_model = PS3_PSW_PRODUCT_MODEL;
	} else {
		instance->product_model = PS3_PRODUCT_MODEL;
	}
	return;
}

static struct ps3_instance *ps3_instance_create(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct Scsi_Host *host = NULL;
	struct ps3_instance *instance = NULL;

	if (pdev == NULL) {
		LOG_ERROR( "pci_dev is null!\n");
		return NULL;
	}

	host = scsi_host_alloc(&ps3_scsi_host_template,
				sizeof(struct ps3_instance));

	if (!host) {
		LOG_ERROR( "pci_id[%u] scsi_host_alloc failed!\n", pdev->dev.id);
		return NULL;
	}

	host->unique_id = PS3_HOST_UUIID(pdev);
	instance = (struct ps3_instance *)shost_priv(host);
	memset(instance, 0, sizeof(*instance));

	instance->pdev = pdev;
	instance->host = host;
	instance->peer_instance = NULL;
	ps3_set_product_model(instance, id->device);
	instance->dev_id = id->device;
	instance->state_machine.can_hostreset = PS3_FALSE;
	instance->pci_err_handle_state = PS3_DEVICE_ERR_STATE_CLEAN;
	mb();

	if (ps3_instance_add(instance) != PS3_SUCCESS) {
		scsi_remove_host(host);
		scsi_host_put(host);
		instance = NULL;
	}

	LOG_INFO("hno:%u scsi_host_alloc [unique_id:%d]!\n",
		PS3_HOST(instance), host->unique_id);
	return instance;
}

static inline void ps3_instance_put(struct ps3_instance *instance)
{
	if (ps3_instance_remove(instance) != PS3_SUCCESS) {
		LOG_ERROR("ps3_instance remove NOK\n");
	}
	return;
}

S32 ps3_pci_init(struct pci_dev *pdev, struct ps3_instance *instance)
{
	resource_size_t base_addr = 0;

	if (pci_enable_device_mem(pdev)) {
		LOG_ERROR("hno:%u pci id[%u] pci enable failed\n",
			PS3_HOST(instance), pdev->dev.id);
		goto l_pci_enable_device_mem_failed;
	}

	pci_set_master(pdev);

	instance->reg_bar = PS3_REGISTER_BAR_INDEX;
	if (!(pci_resource_flags(pdev, instance->reg_bar) & IORESOURCE_MEM)) {
		LOG_ERROR("hno:%u Bar %lu isnot IORESOURCE_MEM\n",
			PS3_HOST(instance), instance->reg_bar);
		goto l_bar_check_failed;
	}
	if (pci_request_selected_regions(pdev, 1<<instance->reg_bar,
		"PS3 pci regions")) {
		LOG_ERROR("hno:%u IO memory region busy\n",
			PS3_HOST(instance));
		goto l_pci_request_selected_regions_failed;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
	pci_enable_pcie_error_reporting(pdev);
#endif
	if (instance->ioc_adpter->reg_set) {
		instance->reg_set =
			(Ps3Fifo_s __iomem *)instance->ioc_adpter->reg_set(pdev,
				instance->reg_bar);
	} else {
		instance->reg_set =
			(Ps3Fifo_s __iomem *)ioremap(
				pci_resource_start(pdev, instance->reg_bar),
                		PS3_REGISTER_SET_SIZE);
	}
	if (instance->reg_set == NULL) {
		LOG_ERROR("hno:%u ioremap failed\n", PS3_HOST(instance));
		goto l_ioremap_failed;
	} else {
		pci_set_drvdata(pdev, instance);
	}

	ps3_atomic_set(&instance->watchdog_reg_read_fail_count, 0);
	LOG_INFO("reg_bar_idx = %lu, bar_base_paddr = 0x%llx, reg_set_vaddr = 0x%p\n", instance->reg_bar,
		(U64)base_addr, instance->reg_set);

	return PS3_SUCCESS;
l_ioremap_failed:
	pci_release_selected_regions(instance->pdev, 1<<instance->reg_bar);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
	pci_disable_pcie_error_reporting(pdev);
#endif
l_pci_request_selected_regions_failed:
	pci_disable_device(instance->pdev);
l_bar_check_failed:
l_pci_enable_device_mem_failed:
	return -PS3_FAILED;
}

static void ps3_pci_exit(struct ps3_instance *instance)
{
	if (instance->reg_set != NULL) {
		iounmap(instance->reg_set);
		instance->reg_set = NULL;
	}

	if (pci_is_enabled(instance->pdev)) {
		pci_release_selected_regions(instance->pdev, 1<<instance->reg_bar);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
		pci_disable_pcie_error_reporting(instance->pdev);
#endif
		pci_disable_device(instance->pdev);
	}

	return;
}

static S32 ps3_scsi_init(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	struct Scsi_Host *host = instance->host;
	host->can_queue = instance->cmd_attr.cur_can_que;
	host->this_id = PS3_SCSI_DEFAULT_INIT_ID;
	host->sg_tablesize = instance->cmd_context.max_host_sge_count;
	host->max_sectors = instance->ctrl_info.maxSectors;
	host->cmd_per_lun = PS3_SCSI_DEFAULT_CMD_PER_LUN;
	host->max_channel = instance->ctrl_info.channelInfo.channelNum - 1;
	host->max_id = instance->dev_context.max_dev_per_channel;
	host->max_lun = PS3_FRAME_LUN_BUFLEN;
	host->max_cmd_len = PS3_FRAME_CDB_BUFLEN;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	host->use_clustering = instance->use_clusting;
#endif

	if (instance->ioc_adpter->sas_transport_get != NULL) {
		host->transportt = instance->ioc_adpter->sas_transport_get();
		host->transportt->user_scan = ps3_sas_user_scan;
	}

#if defined(PS3_TAGSET_SUPPORT)
	host->host_tagset = 0;
	host->nr_hw_queues = 1;

	if ((instance->irq_context.valid_msix_vector_count >
		instance->irq_context.high_iops_msix_vectors) &&
		ps3_tagset_enable_query() &&
		(instance->smp_affinity_enable)) {
		host->host_tagset = 1;
		host->nr_hw_queues = instance->irq_context.valid_msix_vector_count -
			instance->irq_context.high_iops_msix_vectors;
	}
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)

	ret = scsi_init_shared_tag_map(host, host->can_queue);
	if (ret) {
		LOG_ERROR("hno:%u Failed to shared tag from\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}
#endif
	if (scsi_add_host(instance->host, &instance->pdev->dev)) {
		LOG_ERROR("hno:%u Failed to add host\n",
			PS3_HOST(instance));
		ret = -PS3_FAILED;
		goto l_out;
	}

	instance->is_host_added = PS3_TRUE;
l_out:

	return ret;
}

static inline S32 __ps3_config_dma_mask32(struct ps3_instance *instance)
{
	if (dma_set_mask(&instance->pdev->dev, DMA_BIT_MASK(DMA_MASK_BIT_32)) ||
		dma_set_coherent_mask(&instance->pdev->dev,
		DMA_BIT_MASK(DMA_MASK_BIT_32))) {
		LOG_ERROR("hno:%u 32 dma mask set failed\n",
			PS3_HOST(instance));
		return -PS3_FAILED;
	} else {
		instance->dma_mask = DMA_MASK_BIT_32;
		LOG_INFO("hno:%u 32 dma mask set\n",
			PS3_HOST(instance));
	}

	return PS3_SUCCESS;
}

static inline U8 ps3_dma_mask_get(struct ps3_instance *instance)
{
	return ((instance->dma_addr_bit_pos > DMA_MASK_BIT_52) ? DMA_MASK_BIT_52 : DMA_MASK_BIT_44);
}

static inline S32 __ps3_config_dma_mask(struct ps3_instance *instance)
{
	U8 dma_mask = 0;
	dma_mask = ps3_dma_mask_get(instance);
	if (dma_set_mask(&instance->pdev->dev, DMA_BIT_MASK(dma_mask)) ||
		dma_set_coherent_mask(&instance->pdev->dev,
		DMA_BIT_MASK(dma_mask))) {
		LOG_ERROR("hno:%u 62 dma mask set failed\n",
			PS3_HOST(instance));
		return __ps3_config_dma_mask32(instance);
	} else {
		instance->dma_mask = dma_mask;
		LOG_INFO("hno:%u dma mask set %u\n",
			PS3_HOST(instance), instance->dma_mask);
	}
	return PS3_SUCCESS;
}

static S32 ps3_config_dma_mask(struct ps3_instance *instance)
{
	S32 ret = -PS3_FAILED;
	Bool is_dma64_support = PS3_FALSE;
	if (!ps3_ioc_mgr_is_dma64_support(instance, &is_dma64_support)) {
		goto l_out;
	}
	if (is_dma64_support) {
		if (IS_DMA64) {
			ret = __ps3_config_dma_mask(instance);
		} else {
			LOG_ERROR("hno:%u unsupport 32bit system\n",
				PS3_HOST(instance));
		}
	} else {
		LOG_ERROR("hno:%u soc unsupport 64bit dma\n",
			PS3_HOST(instance));
	}
l_out:
	return ret;
}

static inline void ps3_device_busy_threshold_cfg(struct ps3_instance *instance) 
{
	S32 device_busy = 0;

	device_busy = ps3_device_busy_threshold_query();
	if (device_busy <= 0 || 
			device_busy > instance->cmd_attr.cur_can_que) {
		instance->device_busy_threshold = PS3_DEVICE_IO_BUSY_THRESHOLD;
	} else {
		instance->device_busy_threshold = device_busy;
	}
}

static void ps3_cmd_attr_context_init(struct ps3_instance *instance)
{
	U32 cmd_qdepth = ps3_throttle_qdepth_query();

	instance->cmd_attr.cur_can_que =
		instance->cmd_context.max_scsi_cmd_count -
		instance->cmd_context.max_r1x_cmd_count;

	if ((cmd_qdepth != 0) &&
		cmd_qdepth <= (U32)instance->cmd_attr.cur_can_que) {
		instance->cmd_attr.throttle_que_depth = cmd_qdepth;
	} else {
		instance->cmd_attr.throttle_que_depth =
			PS3_DEVICE_QDEPTH_DEFAULT_VALUE;
	}

	instance->cmd_attr.vd_io_threshold = 0;
	instance->cmd_attr.is_support_direct_cmd = PS3_FALSE;

	ps3_device_busy_threshold_cfg(instance);
}

static S32 ps3_init_ioc_prepare(struct ps3_instance *instance)
{
	ps3_ioc_mgr_req_queue_lock_init(instance);
	instance->watchdog_context.is_stop = PS3_TRUE;
	ps3_atomic_set(&instance->watchdog_ref, 0);
	ps3_err_fault_context_init(instance);

	if (!ps3_ioc_fw_version_get(instance)) {
		goto l_get_reg_failed;
	}

	if (!ps3_feature_support_reg_get(instance)) {
		goto l_get_reg_failed;
	}

	if (ps3_ioc_init_cmd_context_init(instance) != PS3_SUCCESS) {
		goto l_ioc_init_failed;
	}

	if (ps3_ctrl_info_buf_alloc(instance) != PS3_SUCCESS) {
		goto l_ctrl_info_init_failed;
	}

	if (ps3_cmd_context_init(instance) != PS3_SUCCESS) {
		goto l_cmd_context_init_failed;
	}

	ps3_cmd_attr_context_init(instance);

	if (ps3_event_context_init(instance) != PS3_SUCCESS) {
		goto l_event_context_init_failed;
	}

    if (ps3_webSubscribe_context_init(instance) != PS3_SUCCESS) {
        goto l_web_context_init_failed;
    }

	if (ps3_cmd_statistics_init(instance) != PS3_SUCCESS) {
		goto l_cmd_stat_failed;
	}

	if (ps3_debug_mem_alloc(instance) != PS3_SUCCESS) {
		goto l_debug_mem_failed;
	}

	if (ps3_dump_init(instance) != PS3_SUCCESS) {
		goto l_dump_init_failed;
	}

	if (ps3_drv_info_buf_alloc(instance) != PS3_SUCCESS) {
		goto l_drv_info_init_failed;
	}

	if (ps3_host_mem_info_buf_alloc(instance) != PS3_SUCCESS) {
		goto l_host_mem_init_failed;
	}
	return PS3_SUCCESS;

l_host_mem_init_failed:
	ps3_drv_info_buf_free(instance);
l_drv_info_init_failed:
	ps3_dump_exit(instance);
l_dump_init_failed:
	ps3_debug_mem_free(instance);
l_debug_mem_failed:
	ps3_cmd_statistics_exit(instance);
l_cmd_stat_failed:
    ps3_webSubscribe_context_exit(instance);
l_web_context_init_failed:
	ps3_event_context_exit(instance);
l_event_context_init_failed:
	ps3_cmd_context_exit(instance);
l_cmd_context_init_failed:
	ps3_ctrl_info_buf_free(instance);
l_ctrl_info_init_failed:
	ps3_ioc_init_cmd_context_exit(instance);
l_ioc_init_failed:
	ps3_recovery_context_exit(instance);
l_get_reg_failed:
	return -PS3_FAILED;
}

static void ps3_init_ioc_prepare_exit(struct ps3_instance *instance)
{
	(void)ps3_debug_mem_free(instance);
	ps3_cmd_statistics_exit(instance);
	ps3_event_context_exit(instance);
    ps3_webSubscribe_context_exit(instance);
	ps3_ctrl_info_buf_free(instance);
	ps3_ioc_init_cmd_context_exit(instance);
	ps3_cmd_context_exit(instance);
	ps3_recovery_context_exit(instance);
	ps3_err_fault_context_exit(instance);
	ps3_dump_dma_buf_free(instance);
	ps3_dump_exit(instance);
	ps3_drv_info_buf_free(instance);
	ps3_host_mem_info_buf_free(instance);
}

static S32 ps3_init_ioc_complete(struct ps3_instance *instance)
{
	if (ps3_mgr_cmd_init(instance) != PS3_SUCCESS) {
		goto l_mgr_cmd_init_failed;
	}

	if (ps3_device_mgr_init(instance) != PS3_SUCCESS) {
		goto l_device_mgr_init_failed;
	}

	if (ps3_sas_device_mgr_init(instance) != PS3_SUCCESS) {
		goto l_sas_device_mgr_init_failed;
	}

	if (ps3_qos_init(instance) != PS3_SUCCESS) {
		goto l_qos_init_failed;
	}

	return PS3_SUCCESS;
l_qos_init_failed:
    ps3_sas_device_mgr_exit(instance);
l_sas_device_mgr_init_failed:
	ps3_device_mgr_exit(instance);
l_device_mgr_init_failed:
	ps3_mgr_cmd_exit(instance);
l_mgr_cmd_init_failed:
	return -PS3_FAILED;
}

static void ps3_init_ioc_complete_exit(struct ps3_instance *instance)
{
	ps3_sas_device_mgr_exit(instance);
	ps3_device_mgr_exit(instance);
	ps3_mgr_cmd_exit(instance);

	ps3_qos_exit(instance);

	return ;
}

S32 ps3_pci_init_complete(struct ps3_instance *instance)
{
	if (ps3_config_dma_mask(instance) != PS3_SUCCESS) {
		goto l_failed;
	}

	if (ps3_irq_context_init(instance) != PS3_SUCCESS) {
		goto l_failed;
	}
	if (instance->ioc_adpter->irq_init) {
		if (instance->ioc_adpter->irq_init(instance) != PS3_SUCCESS) {
			goto l_irqs_init_failed;
		}
	} else {
		if (ps3_irqs_init(instance) != PS3_SUCCESS) {
			goto l_irqs_init_failed;
		}
	}

	return PS3_SUCCESS;
l_irqs_init_failed:
	ps3_irq_context_exit(instance);
l_failed:
	return -PS3_FAILED;
}

static S32 ps3_wait_reg_access_done(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 read_count = 0;
	const U32 retry_max  = 9000;

	ps3_wait_scsi_cmd_done(instance, PS3_TRUE);
	while (ps3_atomic_read(&instance->reg_op_count) != 0){
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_20MS);

		if(read_count++ > retry_max){
			LOG_INFO("hno:%u  wait reg op over:%d ms, failed\n",
				PS3_HOST(instance), read_count*PS3_LOOP_TIME_INTERVAL_20MS);
		    ret = -PS3_FAILED;
			goto l_out;
		}
	}

l_out:
	return ret;
}

void ps3_pci_init_complete_exit(struct ps3_instance *instance)
{
	ps3_irqs_exit(instance);

	instance->is_pci_reset = PS3_TRUE;
	mb();
	if (ps3_wait_reg_access_done(instance) != PS3_SUCCESS) {
		LOG_ERROR("hno:%u wait reg access done NOK\n", PS3_HOST(instance));
		return;
	}

	ps3_pci_exit(instance); 

	if (!ps3_pci_err_recovery_get(instance) &&
		!instance->state_machine.is_suspend) {
		pci_set_drvdata(instance->pdev, NULL);
	}

	ps3_irq_context_exit(instance);
}

static Bool ps3_bit_pos_set(struct ps3_instance *instance)
{
	U8 bit_pos = 0;
	Bool ret = PS3_TRUE;

	if (!ps3_ioc_atu_support_retry_read(instance, &bit_pos)) {
		ret = PS3_FALSE;
		goto l_out;
	}
	if (bit_pos <= PS3_BIT_POS_44) {
		instance->dma_addr_bit_pos = PCIE_DMA_HOST_ADDR_BIT_POS;
		goto l_out;
	}
	if (ps3_get_pci_function(instance->pdev) == PS3_FUNC_ID_0) {
		instance->dma_addr_bit_pos = PCIE_DMA_HOST_ADDR_BIT_POS_F0;
	} else {
		instance->dma_addr_bit_pos = PCIE_DMA_HOST_ADDR_BIT_POS_F1;
	}
l_out:
	mb();
	LOG_WARN("hno:%u set bit pos %u\n",
			PS3_HOST(instance), instance->dma_addr_bit_pos);
	return ret;
}

static S32 ps3_firmware_init(struct pci_dev *pdev, struct ps3_instance *instance)
{
	S32 ret;

	if (ps3_recovery_context_init(instance) != PS3_SUCCESS) {
		goto l_recovery_context_init_failed;
	}

	if (ps3_pci_init(pdev, instance) != PS3_SUCCESS) {
		goto l_failed;
	}

	if (!ps3_ioc_recovery_count_get(instance,
		&instance->recovery_context->ioc_recovery_count)) {
		goto l_failed;
	}

	INJECT_START(PS3_ERR_IJ_SET_IOC_IN_SECURITY, instance)
	if (instance->ioc_adpter->ioc_security_check &&
				instance->ioc_adpter->ioc_security_check(instance)) {
		LOG_WARN("hno:%u:ioc is in security state\n", PS3_HOST(instance));
		goto l_failed;
	}
	INJECT_START(PS3_ERR_IJ_RECOVERY_F1_B_READY, instance)
	if (instance->ioc_adpter->ioc_init_state_to_ready(instance) != PS3_SUCCESS) {
		goto l_failed;
	}
	atomic_set(&instance->state_machine.state, PS3_INSTANCE_STATE_READY);
	INJECT_START(PS3_ERR_IJ_RECOVERY_F1_A_READY, instance);
	if (!ps3_bit_pos_set(instance)) {
		goto l_failed;
	}
	if (ps3_pci_init_complete(instance) != PS3_SUCCESS) {
		goto l_failed;
	}
	instance->pci_err_handle_state = PS3_DEVICE_ERR_STATE_NORMAL;

	pci_save_state(pdev);
	ret = ps3_init_ioc_prepare(instance);
	INJECT_START(PS3_ERR_IJ_FIRMWARE_INIT_FAIL, &ret)
	if (ret != PS3_SUCCESS) {
		goto l_failed;
	}

	INJECT_START(PS3_ERR_IJ_RECOVERY_F1_B_INIT, instance)
	if (instance->ioc_adpter->ioc_init_proc(instance) != PS3_SUCCESS) {
		goto l_failed;
	}
	atomic_set(&instance->state_machine.state, PS3_INSTANCE_STATE_PRE_OPERATIONAL);
	INJECT_START(PS3_ERR_IJ_RECOVERY_F1_CTRL_INFO, instance)

	if (ps3_ctrl_info_get(instance) != PS3_SUCCESS) {
		goto l_failed;
	}

	if (ps3_init_ioc_complete(instance) != PS3_SUCCESS) {
		goto l_failed;
	}

	return PS3_SUCCESS;

l_failed:
	ps3_recovery_context_exit(instance);
l_recovery_context_init_failed:
	ps3_pci_init_complete_exit(instance);
	ps3_init_ioc_prepare_exit(instance);
	return -PS3_FAILED;
}

static void ps3_firmware_exit(struct ps3_instance *instance)
{
	ps3_pci_init_complete_exit(instance);
	ps3_init_ioc_complete_exit(instance);
	ps3_init_ioc_prepare_exit(instance);
	return;
}

static int ps3_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct ps3_instance *instance = NULL;
	ULong flags = 0;
	S32 ret;

#ifdef PS3_SUPPORT_INJECT
	U32 times = 180000;
#endif
#ifdef PS3_HARDWARE_ASIC
	U16 real_dev_id = 0;
	U32 check_count = ps3_hba_check_time_query() * 10 + 1;
#endif
	LOG_INFO("device[%d] ps3_probe\n", pdev->dev.id);

	if (ps3_avaliable_func_id_query() >= PS3_FUNC_UNLIMITED) {
		LOG2_WARN("PCI Device %04x:%02x:%02x:%x probe start.\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
			ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));
	} else {
		if ((U32)ps3_get_pci_function(pdev) != ps3_avaliable_func_id_query()) {
			LOG2_WARN("Function %x not avaliable\n", ps3_get_pci_function(pdev));
			goto l_func_id_error;
		} else {
			LOG2_WARN("PCI Device %04x:%02x:%02x:%x probe start.\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
				ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));
		}
	}

	INJECT_INIT()
	INJECT_PROBE_ACTIVE()

	instance = ps3_instance_create(pdev, id);
	if (instance == NULL) {
		goto l_ps3_instance_create_failed;
	}

	if (instance->peer_instance != NULL) {
		ps3_recovery_cancel_work_sync(instance->peer_instance);
	}

#ifdef PS3_HARDWARE_ASIC
	while(id->device == PCI_DEVICE_ID_PS3_RAID_FPGA &&
			check_count > 0) {
		pci_read_config_word(pdev, PCI_DEVICE_ID, &real_dev_id);
		LOG_INFO("get real device id is[0x%x] \n", real_dev_id);
		if (real_dev_id == PCI_DEVICE_ID_PS3_HBA ||
				real_dev_id == PCI_DEVICE_ID_STARS_IOC_2020_18i ||
				real_dev_id == PCI_DEVICE_ID_STARS_ROC_2020_10i) {
			((struct pci_device_id *)id)->device = real_dev_id;
			break;
		} else {
			check_count--;
			ps3_msleep(100);
		}

	};
#endif
	ps3_ioc_adp_init(instance, id);
	instance->ioc_adpter->ioc_resource_prepare(instance);
	ps3_debug_context_init(instance);

	device_disable_async_suspend(&pdev->dev);

	ret = ps3_firmware_init(pdev, instance);
	if (ret != PS3_SUCCESS) {
		LOG_WARN("ps3_firmware_init fail:%d\n", ret);
		goto l_firmware_init_failed;
	}
	ps3_atomic_set(&instance->state_machine.state, PS3_INSTANCE_STATE_OPERATIONAL);
	INJECT_START(PS3_ERR_IJ_RECOVERY_F1_OPER, instance)

	instance->ioc_adpter->irq_enable(instance);
	ps3_dump_ctrl_set_int_ready(instance);

	ps3_ioctl_init(instance, PS3_MAX_IOCTL_CMDS);
	ps3_scsi_cmd_timeout_adjust();
	INJECT_START(PS3_ERR_IJ_RECOVERY_F1_SCSI_INIT, instance)

	ret = ps3_scsi_init(instance);
	if (ret != PS3_SUCCESS) {
		goto l_scsi_init_failed;
	}
	instance->state_machine.is_load = PS3_TRUE;
	mb();
	INJECT_START(PS3_ERR_IJ_FORCE_F1_LOAD_START_HARD, instance);
	INJECT_START(PS3_ERR_IJ_RECOVERY_F1_LOAD, instance)
	ret = ps3_watchdog_start(instance);
	if (ret != PS3_SUCCESS) {
		goto l_watch_dog_start_failed;
	}
	INJECT_START(PS3_ERR_IJ_F1_PROBE_FORCE_IOC_FAULT, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_HOST_START_HARD, instance)

	ret = ps3_device_mgr_data_init(instance);
	if (ret != PS3_SUCCESS) {
		goto l_dev_info_get_failed;
	}
	ret = ps3_sas_device_data_init(instance);
	if (ret != PS3_SUCCESS) {
		goto l_expander_get_failed;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_F1_SCAN_START_HARD, instance)
	INJECT_START(PS3_ERR_IJ_RECOVERY_F1_B_SCAN, instance)
	instance->state_machine.can_hostreset = PS3_TRUE;
	INJECT_START(PS3_ERR_IJ_FORCE_F0_WATCHDOG_START_HARD, instance)

	ps3_scsi_scan_host(instance);

	INJECT_START(PS3_ERR_IJ_RECOVERY_F1_A_SCAN, instance)
	INJECT_START(PS3_ERR_IJ_SCSI_SCAN_HOST_DELAY, &times)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_A_SCAN_START_HARD, instance)

	INJECT_START(PS3_ERR_IJ_PROBE_HOST_RESET, instance);
	instance->is_scan_host_finish = PS3_TRUE;
	INJECT_START(PS3_ERR_IJ_RECOVERY_F1_SCAN_FINISH, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_A_SCAN_FINISH_HARD, instance)

	ret = ps3_event_subscribe(instance);
	if (ret == -PS3_FAILED) {
		goto l_event_subscribe_failed;
	} else if (ret == -PS3_RECOVERED) {
		LOG_INFO("device[%d] skip event/vd info subscribe due to recovery during probe\n",
			pdev->dev.id);
		goto l_event_subscribe_recovered;
	}
	INJECT_START(PS3_ERR_IJ_RECOVERY_F1_EVENT, instance)

	ret = ps3_dev_mgr_vd_info_subscribe(instance);
	if (ret != PS3_SUCCESS && ret != -PS3_RECOVERED) {
		LOG_INFO("device[%d] vd info subscribe failed, ret: %d\n",
			pdev->dev.id, ret);
		goto l_vd_info_subscribe_failed;
	}
l_event_subscribe_recovered:

	instance->is_probe_finish = PS3_TRUE;
	mb();
#if 0
	if (ps3_watchdog_start(instance) != PS3_SUCCESS) {
		goto l_watch_dog_start_failed;
	}
#endif
	LOG2_WARN("PCI Device %04x:%02x:%02x:%x probe end.\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
		ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));

	ps3_ioc_can_hardreset_set(instance, PS3_IOC_CAN_HARDRESET);
	return PS3_SUCCESS;

l_vd_info_subscribe_failed:
l_event_subscribe_failed:
	ps3_dev_mgr_vd_info_unsubscribe(instance);
	ps3_event_unsubscribe(instance);
	ps3_spin_lock_irqsave(&instance->recovery_context->recovery_lock, &flags);
	instance->event_context.abort_eventcmd = 0;
	instance->dev_context.abort_vdpending_cmd = 0;
	ps3_spin_unlock_irqrestore(&instance->recovery_context->recovery_lock, flags);
	ps3_sas_device_data_exit(instance);
l_expander_get_failed:
l_scsi_init_failed:
	instance->state_machine.is_load = PS3_FALSE;
	mb();
	ps3_device_mgr_data_exit(instance);
l_dev_info_get_failed:
	ps3_watchdog_stop(instance);
l_watch_dog_start_failed:
	if (ps3_sas_is_support_smp(instance)) {
		sas_remove_host(instance->host);
		scsi_remove_host(instance->host);
	} else {
		scsi_remove_host(instance->host);
	}
	instance->state_machine.can_hostreset = PS3_FALSE;
	mb();
	ps3_dump_work_stop(instance);
	ps3_recovery_context_exit(instance);
	instance->ioc_adpter->irq_disable(instance);
	ps3_irqs_sync(instance);

	ps3_irqpolls_enable(instance);
	ps3_firmware_exit(instance);
l_firmware_init_failed:
	instance->is_probe_failed = PS3_TRUE;
	mb();
	ps3_instance_put(instance);
	scsi_host_put(instance->host);
l_ps3_instance_create_failed:
	ps3_dma_dump_mapping(pdev);
 l_func_id_error:
	printk(KERN_WARNING "PCI Device %04x:%02x:%02x:%x probe failed.\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
		ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));
	return -ENODEV;
}

static S32 ps3_cancel_event_wk_in_unload(struct ps3_instance *instance)
{
	S32 ret = PS3_SUCCESS;
	U32 cur_state = 0;
	U32 ioc_state = 0;

	cur_state = ps3_atomic_read(&instance->state_machine.state);
	ioc_state = instance->ioc_adpter->ioc_state_get(instance);
	INJECT_START(PS3_ERR_IJ_IOC_NOT_RUNNING, &ioc_state)
	if((ioc_state != PS3_FW_STATE_RUNNING) ||
		(cur_state != PS3_INSTANCE_STATE_OPERATIONAL)){
		LOG_WARN("hno:%u  goto half hard reset, cur_state:%s,IOC state is %s!\n",
			PS3_HOST(instance),
			namePS3InstanceState(cur_state),ps3_ioc_state_print(ioc_state));
		ret = -PS3_FAILED;
		goto l_out;
	}
	cancel_delayed_work_sync(&instance->event_context.delay_work->event_work);

l_out:
	return ret;
}

static void ps3_shutdown(struct pci_dev *pdev)
{
	struct ps3_instance *instance = (struct ps3_instance *)pci_get_drvdata(pdev);
	if (instance == NULL) {
		LOG_ERROR("device[%d] ps3_shutdown fail\n", pdev->dev.id);
		return;
	}
	LOG2_WARN("PCI Device %04x:%02x:%02x:%x shutdown start.\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
		ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));

	instance->state_machine.is_load = PS3_FALSE;
	mb(); 
	INJECT_START(PS3_ERR_IJ_SHUTDOWN_HARDRESET, instance)
	ps3_recovery_cancel_work_sync(instance);
	if (instance ->is_half_hard_reset) {
		goto l_reset_to_ready;
	}

	ps3_r1x_conflict_queue_clean_all(instance,
		PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT), PS3_TRUE);

	ps3_qos_waitq_clear_all(instance, PS3_STATUS_HOST_NOT_FOUND);
#ifdef PS3_SUPPORT_INJECT
	ps3_inject_clear();
#endif

	if (ps3_cancel_event_wk_in_unload(instance) != PS3_SUCCESS) {
		goto l_reset_to_ready;
	}

	if (ps3_event_unsubscribe(instance) != PS3_SUCCESS) {
		LOG_ERROR("device[%d] event unsubscribe NOK.\n", pdev->dev.id);
		goto l_reset_to_ready;
	}

	if (ps3_dev_mgr_vd_info_unsubscribe(instance) != PS3_SUCCESS) {
		LOG_ERROR("device[%d] vd unsubscribe NOK.\n", pdev->dev.id);
		goto l_reset_to_ready;
	}

    if (ps3_atomic_read(&instance->webSubscribe_context.is_subscribe) == 1) {
        while (ps3_atomic_read(&instance->cmd_statistics.cmd_delivering) != 0) {
            ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
        }
        if (ps3_web_unsubscribe(instance) != PS3_SUCCESS) {
            LOG_ERROR("device[%d] web unsubscribe NOK.\n", pdev->dev.id);
            goto l_reset_to_ready;
        }
    }

	instance->is_host_added = PS3_FALSE;
	ps3_watchdog_stop(instance);

	ps3_dump_exit(instance);

	INJECT_START(PS3_ERR_IJ_HALF_HARD_RESET, &instance ->is_half_hard_reset)
	if (instance ->is_half_hard_reset) {
		goto l_release_res;
	}

	if (ps3_soc_unload(instance, PS3_FALSE, PS3_UNLOAD_SUB_TYPE_SHUTDOWN, PS3_SUSPEND_TYPE_NONE) != PS3_SUCCESS) {
		LOG_ERROR("device[%d] unload failed.\n", pdev->dev.id);
		goto l_reset_to_ready;
	}

	goto l_release_res;
l_reset_to_ready:
	if (instance->is_host_added) {
		ps3_watchdog_stop(instance);
		ps3_dump_exit(instance);
		if (instance ->is_half_hard_reset) {
			goto l_release_res;
		}
	}

	if (ps3_hard_reset_to_ready_with_doorbell(instance) !=
			PS3_SUCCESS) {
		LOG_ERROR("device[%d] hard reset NOK.\n",
		PS3_HOST(instance));
	}
l_release_res:
	while(atomic_read(&instance->cmd_statistics.io_outstanding) != 0) {
		ps3_msleep(3000);
	}
	instance->state_machine.can_hostreset = PS3_FALSE;
	ps3_check_and_wait_host_reset(instance);
	if (!instance ->is_half_hard_reset) {
		instance->ioc_adpter->irq_disable(instance);
		ps3_irqs_sync(instance);
	}
	ps3_recovery_destory(instance);

	ps3_instance_state_transfer_to_quit(instance);
	ps3_recovery_clean(instance);

	ps3_ioctl_clean(instance);

	ps3_firmware_exit(instance);
	ps3_instance_put(instance);
	LOG2_WARN("PCI Device %04x:%02x:%02x:%x shutdown end.\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
		ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));
	return;
}

void ps3_remove(struct pci_dev *pdev)
{
	struct ps3_instance *instance = (struct ps3_instance *)pci_get_drvdata(pdev);
	if (instance == NULL) {
		LOG_ERROR("device[%d] ps3_remove fail\n", pdev->dev.id);
		return;
	}
	instance->state_machine.is_load = PS3_FALSE;
	LOG2_WARN("PCI Device %04x:%02x:%02x:%x remove start\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
		ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));
	if (instance->pci_err_handle_state == PS3_DEVICE_ERR_STATE_CLEAN) {
		ps3_recovery_cancel_work_sync(instance);
		ps3_dump_exit(instance);
		while(ps3_atomic_read(&instance->is_err_scsi_processing) > 0) {
			ps3_msleep(10);
		}
		
		ps3_all_reply_fifo_complete(instance);
		
		ps3_scsi_cmd_force_stop(instance);
		ps3_mgr_cmd_force_stop(instance);
		cancel_delayed_work_sync(&instance->event_context.delay_work->event_work);

		goto l_release_res;
	}
	mb(); 
	ps3_recovery_cancel_work_sync(instance);
	if (instance ->is_half_hard_reset) {
		goto l_reset_to_ready;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_IOC_NOT_RUNNING, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_REQ_IOC_CANCEL_EVENT_WORK_FAIL_RESUME, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_IOC_CANCEL_EVENT_WORK_FAIL_RESUME, instance)
	if (ps3_cancel_event_wk_in_unload(instance) != PS3_SUCCESS) {
		goto l_reset_to_ready;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_IOC_EVENT_FAIL, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_REQ_IOC_EVENT_FAIL_RESUME, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_IOC_EVENT_FAIL_RESUME, instance)
	if (ps3_event_unsubscribe(instance) != PS3_SUCCESS) {
		LOG_ERROR("device[%d] event unsubscribe NOK.\n", pdev->dev.id);
		goto l_reset_to_ready;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_IOC_VDINFO_FAIL, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_REQ_IOC_VDINFO_FAIL_RESUME, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_IOC_VDINFO_FAIL_RESUME, instance)
	if (ps3_dev_mgr_vd_info_unsubscribe(instance) != PS3_SUCCESS) {
		LOG_ERROR("device[%d] vd unsubscribe NOK.\n", pdev->dev.id);
		goto l_reset_to_ready;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_IOC_WEB_FAIL, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_REQ_IOC_WEB_FAIL_RESUME, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_IOC_WEB_FAIL_RESUME, instance)
    if (ps3_atomic_read(&instance->webSubscribe_context.is_subscribe) == 1) {
        while (ps3_atomic_read(&instance->cmd_statistics.cmd_delivering) != 0) {
            ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
        }
        if (ps3_web_unsubscribe(instance) != PS3_SUCCESS) {
            LOG_ERROR("device[%d] web unsubscribe NOK.\n", pdev->dev.id);
            goto l_reset_to_ready;
        }
    }

	ps3_r1x_conflict_queue_clean_all(instance,
		PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT), PS3_TRUE);

	ps3_qos_waitq_clear_all(instance, PS3_STATUS_HOST_NOT_FOUND);
#ifdef PS3_SUPPORT_INJECT
	ps3_inject_clear();
#endif

	if (ps3_sas_is_support_smp(instance)) {
		sas_remove_host(instance->host);
		scsi_remove_host(instance->host);
	} else {
		scsi_remove_host(instance->host);
	}

	instance->is_host_added = PS3_FALSE;

	ps3_watchdog_stop(instance);
	ps3_dump_exit(instance);

	INJECT_START(PS3_ERR_IJ_HALF_HARD_RESET, &instance ->is_half_hard_reset)
	if (instance ->is_half_hard_reset) {
		goto l_release_res;
	}
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_IOC_UNLOAD_FAIL, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_REQ_IOC_UNLOAD_FAIL_RESUME, instance)
	INJECT_START(PS3_ERR_IJ_FORCE_F1_REMOVE_RECOVERY_IOC_UNLOAD_FAIL_RESUME, instance)
	if (ps3_soc_unload(instance, PS3_FALSE, PS3_UNLOAD_SUB_TYPE_REMOVE, PS3_SUSPEND_TYPE_NONE) != PS3_SUCCESS) {
		LOG_ERROR("device[%d] unload failed.\n", pdev->dev.id);
		goto l_reset_to_ready;
	}

	goto l_release_res;
l_reset_to_ready:
	if (instance->is_host_added) {
		ps3_watchdog_stop(instance);
		ps3_dump_exit(instance);
		if (instance ->is_half_hard_reset) {
			goto l_release_res;
		}
	}
	if (ps3_hard_reset_to_ready_with_doorbell(instance) !=
			PS3_SUCCESS) {
		LOG_ERROR("device[%d] hard reset NOK.\n",
		PS3_HOST(instance));
	}

l_release_res:
	while(atomic_read(&instance->cmd_statistics.io_outstanding) != 0) {
		ps3_msleep(3000);
	}
	instance->state_machine.can_hostreset = PS3_FALSE;
	ps3_check_and_wait_host_reset(instance);

    ps3_instance_state_transfer_to_quit(instance);
	ps3_recovery_destory(instance);

	if (instance->is_host_added) {
		ps3_r1x_conflict_queue_clean_all(instance,
			PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT), PS3_TRUE);

		ps3_qos_waitq_clear_all(instance, PS3_STATUS_HOST_NOT_FOUND);

		if (ps3_sas_is_support_smp(instance)) {
			sas_remove_host(instance->host);
			scsi_remove_host(instance->host);
		} else {
			scsi_remove_host(instance->host);
		}
	}

	while(ps3_atomic_read(&instance->host_reset_processing) > 0) {
		ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
	}
	ps3_recovery_clean(instance);
	if (!instance ->is_half_hard_reset) {
		instance->ioc_adpter->irq_disable(instance);
		ps3_irqs_sync(instance);
	}
	ps3_ioctl_clean(instance);

	ps3_firmware_exit(instance);

	ps3_instance_put(instance);
	scsi_host_put(instance->host);
	ps3_dma_dump_mapping(pdev);
	LOG2_WARN("PCI Device %04x:%02x:%02x:%x remove end\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
		ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));
	return;
}

#ifdef CONFIG_PM
static int ps3_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct ps3_instance *instance = (struct ps3_instance *)pci_get_drvdata(pdev);
	struct ps3_cmd *cmd = NULL;
	struct ps3_cmd *abort_cmd = NULL;
	U8 suspend_type = PS3_SUSPEND_TYPE_SLEEP;

	if (instance == NULL) {
		LOG2_ERROR("device[%d] ps3_suspend fail\n", pdev->dev.id);
		return PS3_SUCCESS;
	}

	LOG2_WARN("PCI Device %04x:%02x:%02x:%x ps3_suspend start.\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
		ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));
	instance->is_suspend = PS3_TRUE;
	mb();
	instance->state_machine.is_load = PS3_FALSE;
	instance->is_probe_finish = PS3_FALSE;
	instance->state_machine.is_suspend = PS3_TRUE;
	mb(); 
	INJECT_START(PS3_ERR_IJ_WAIT_SUSPEND_WAIT_RECOVERY_1, instance)
	ps3_recovery_cancel_work_sync(instance);

	if (instance ->is_half_hard_reset) {
		goto l_reset_to_ready;
	}

	ps3_r1x_conflict_queue_clean_all(instance,
		PS3_SCSI_RESULT_HOST_STATUS(DID_NO_CONNECT), PS3_TRUE);

	ps3_qos_waitq_clear_all(instance, PS3_STATUS_HOST_NOT_FOUND);
#ifdef PS3_SUPPORT_INJECT
	ps3_inject_clear();
#endif

	if (ps3_cancel_event_wk_in_unload(instance) != PS3_SUCCESS) {
		goto l_reset_to_ready;
	}

	if (ps3_event_unsubscribe(instance) != PS3_SUCCESS) {
		LOG_ERROR("device[%d] event unsubscribe NOK.\n", pdev->dev.id);
		goto l_reset_to_ready;
	}

	if (ps3_dev_mgr_vd_info_unsubscribe(instance) != PS3_SUCCESS) {
		LOG_ERROR("device[%d] vd unsubscribe NOK.\n", pdev->dev.id);
		goto l_reset_to_ready;
	}

	if (ps3_atomic_read(&instance->webSubscribe_context.is_subscribe) == 1) {
		while (ps3_atomic_read(&instance->cmd_statistics.cmd_delivering) != 0) {
			ps3_msleep(PS3_LOOP_TIME_INTERVAL_100MS);
		}
		if (ps3_web_unsubscribe(instance) != PS3_SUCCESS) {
			LOG_ERROR("device[%d] web unsubscribe NOK.\n", pdev->dev.id);
			goto l_reset_to_ready;
		}
	}
	instance->ioc_adpter->irq_disable(instance);
	ps3_irqs_sync(instance);

	ps3_irqpolls_enable(instance);

	ps3_dump_work_stop(instance);
	ps3_watchdog_stop(instance);
	ps3_recovery_cancel_work_sync(instance);

	if (instance ->is_half_hard_reset) {
		goto l_release_res;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,6,0))
	flush_scheduled_work();
#endif
	scsi_block_requests(instance->host);

	if (state.event == PM_EVENT_FREEZE) {
		suspend_type = PS3_SUSPEND_TYPE_HIBERNATE;
	}
	LOG_INFO("event[%d] suspend_type[%d].\n", state.event, suspend_type);
	if (ps3_soc_unload(instance, PS3_FALSE, PS3_UNLOAD_SUB_TYPE_SUSPEND, suspend_type) != PS3_SUCCESS) {
		LOG_ERROR("device[%d] unload failed.\n", pdev->dev.id);
		if (ps3_hard_reset_to_ready_with_doorbell(instance) !=
			PS3_SUCCESS) {
			LOG_ERROR("device[%d] hard reset NOK.\n",
			PS3_HOST(instance));
		}
	}

	goto l_release_res;
l_reset_to_ready:
	if (!instance ->is_half_hard_reset) {
		instance->ioc_adpter->irq_disable(instance);
		ps3_irqs_sync(instance);

		ps3_irqpolls_enable(instance);
	}
	ps3_dump_work_stop(instance);
	ps3_watchdog_stop(instance);
	ps3_recovery_cancel_work_sync(instance);
	if (instance ->is_half_hard_reset) {
		goto l_release_res;
	}

	if (ps3_hard_reset_to_ready_with_doorbell(instance) !=
			PS3_SUCCESS) {
		LOG_ERROR("device[%d] hard reset NOK.\n",
		PS3_HOST(instance));
	}

	if (instance->event_context.event_cmd != NULL) {
		cmd = instance->event_context.event_cmd;
		instance->event_context.event_cmd = NULL;
		ps3_mgr_cmd_free(instance, cmd);
		abort_cmd = instance->event_context.event_abort_cmd;
		if (abort_cmd != NULL) {
			instance->event_context.event_abort_cmd = NULL;
			ps3_task_cmd_free(instance, abort_cmd);
		}
		instance->event_context.subwork = 0;
	}

	ps3_dev_mgr_vd_info_clear(instance);

	if (ps3_atomic_read(&instance->webSubscribe_context.is_subscribe) == 1) {
		ps3_web_cmd_clear(instance);
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,6,0))
	flush_scheduled_work();
#endif
	scsi_block_requests(instance->host);
l_release_res:
	pci_set_drvdata(instance->pdev, instance);
	pci_save_state(pdev);
	instance ->is_half_hard_reset = PS3_FALSE;

	LOG2_WARN("PCI Device %04x:%02x:%02x:%x ps3_suspend end.\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
		ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));
	instance->is_suspend = PS3_FALSE;
	return PS3_SUCCESS;
}

static int ps3_resume(struct pci_dev *pdev)
{
	S32 ret = PS3_SUCCESS;
	struct ps3_instance *instance = (struct ps3_instance *)pci_get_drvdata(pdev);

	if (instance == NULL) {
		LOG2_ERROR("device[%d] ps3_resume fail\n", pdev->dev.id);
		return PS3_SUCCESS;
	}

	LOG2_WARN("PCI Device %04x:%02x:%02x:%x ps3_resume start.\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
		ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));
	instance->is_resume = PS3_TRUE;
	mb();
	ps3_recovery_cancel_work_sync(instance);
	pci_restore_state(pdev);

	if (ps3_avaliable_func_id_query() < PS3_FUNC_UNLIMITED && 
			(U32)ps3_get_pci_function(pdev) != ps3_avaliable_func_id_query()) {
		LOG2_WARN("Function %d not avaliable\n", ps3_get_pci_function(pdev));
		goto l_out;
	}

	instance->pdev = pdev;
	pci_save_state(pdev);

	if (instance->ioc_adpter->ioc_init_state_to_ready(instance) != PS3_SUCCESS) {
		goto l_out;
	}

	atomic_set(&instance->state_machine.state, PS3_INSTANCE_STATE_READY);

	ps3_all_reply_fifo_init(instance);

	if (instance->ioc_adpter->ioc_init_proc(instance) != PS3_SUCCESS) {
		goto l_out;
	}

	atomic_set(&instance->state_machine.state, PS3_INSTANCE_STATE_PRE_OPERATIONAL);

	instance->ioc_adpter->irq_enable(instance);

	ps3_atomic_set(&instance->state_machine.state, PS3_INSTANCE_STATE_OPERATIONAL);

	instance->state_machine.is_load = PS3_TRUE;
	scsi_unblock_requests(instance->host);

	ps3_dump_ctrl_set_int_ready(instance);

	ret = ps3_watchdog_start(instance);
	if (ret != PS3_SUCCESS) {
		goto l_watch_dog_start_failed;
	}

	instance->event_req_info.eventTypeMapProcResult = instance->event_req_info.eventTypeMap;
	ret = ps3_event_subscribe(instance);
	if (ret == -PS3_FAILED) {
		goto l_event_subscribe_failed;
	} else if (ret == -PS3_RECOVERED) {
		LOG_INFO("device[%d] skip event/vd info subscribe due to recovery during probe\n",
			pdev->dev.id);
		goto l_event_subscribe_recovered;
	}

	ret = ps3_dev_mgr_vd_info_subscribe(instance);
	if (ret != PS3_SUCCESS && ret != -PS3_RECOVERED) {
		LOG_INFO("device[%d] vd info subscribe failed, ret: %d\n",
			pdev->dev.id, ret);
		goto l_vd_info_subscribe_failed;
	}

	if (ps3_atomic_read(&instance->webSubscribe_context.is_subscribe) == 1) {
		ret = ps3_web_subscribe(instance);
		if (ret != PS3_SUCCESS && ret != -PS3_IN_UNLOAD) {
			goto l_vd_info_subscribe_failed;
		}
	}

l_event_subscribe_recovered:
	instance->state_machine.is_suspend = PS3_FALSE;
	instance->is_probe_finish = PS3_TRUE;
	instance->is_resume = PS3_FALSE;
	LOG2_WARN("PCI Device %04x:%02x:%02x:%x ps3_resume end.\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
		ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));

	ps3_ioc_can_hardreset_set(instance, PS3_IOC_CAN_HARDRESET);
	return PS3_SUCCESS;
l_vd_info_subscribe_failed:
l_event_subscribe_failed:
	ps3_dev_mgr_vd_info_unsubscribe(instance);
	ps3_event_unsubscribe(instance);
l_watch_dog_start_failed:
	ps3_watchdog_stop(instance);
	ps3_dump_work_stop(instance);
l_out:
	instance->state_machine.is_suspend = PS3_FALSE;
	instance->is_probe_failed = PS3_TRUE;
	instance->is_resume = PS3_FALSE;
	instance->state_machine.is_load = PS3_TRUE;
	scsi_host_put(instance->host);
	ps3_dma_dump_mapping(pdev);
	pci_disable_device(pdev);
	LOG2_WARN("PCI Device %04x:%02x:%02x:%x ps3_resume fail.\n", ps3_get_pci_domain(pdev), ps3_get_pci_bus(pdev), \
		ps3_get_pci_slot(pdev), ps3_get_pci_function(pdev));
	return -ENODEV;
}

int ps3_cli_suspend(struct pci_dev *pdev, pm_message_t state) {
	return ps3_suspend(pdev, state);
}

int ps3_cli_resume(struct pci_dev *pdev) {
	return ps3_resume(pdev);
}

#endif 

static ssize_t version_show(struct device_driver *dd, char *buf)
{
	(void)dd;
	return snprintf(buf, strlen(PS3_DRV_VERSION) + 2, "%s\n", PS3_DRV_VERSION);
}
static DRIVER_ATTR_RO(version);

static ssize_t release_date_show(struct device_driver *dd, char *buf)
{
	(void)dd;
	return snprintf(buf, strlen(PS3_DRV_BUILD_TIME) + 2, "%s\n",
			PS3_DRV_BUILD_TIME);
}
static DRIVER_ATTR_RO(release_date);

static const struct file_operations ps3_mgmt_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ps3_ioctl_fops,
	.open = ps3_mgmt_open,
	.release = NULL,
	.llseek = noop_llseek,
	.read = NULL,
	.write = NULL,
	.fasync = ps3_fasync
};

static struct pci_driver ps3_pci_driver;
static void init_pci_driver(struct pci_driver *drv)
{
	drv->name = PS3_PCI_DRIVER_NAME;
	drv->id_table = ps3_pci_table;
	drv->probe = ps3_probe;
	drv->remove = ps3_remove;
#ifdef CONFIG_PM
	drv->suspend = ps3_suspend;
	drv->resume = ps3_resume;
#endif
	drv->shutdown = ps3_shutdown;
	if (ps3_aer_handle_support_query()) {
		ps3_pci_err_handler_init(drv);
	} else {
		LOG_INFO("aer handle not supported\n");
	}

	return;
}

static int __init ps3stor_init(void)
{
	int ret = PS3_SUCCESS;
	printk(KERN_WARNING "ps3stor driver init start, version[%s], commit_id[%s], "
		"build_time[%s], toolchain_id[%s]\n",
		PS3_DRV_VERSION, PS3_DRV_COMMIT_ID,
		PS3_DRV_BUILD_TIME, PS3_DRV_TOOLCHAIN_ID);

	ret = ps3_debug_init();
	INJECT_START(PS3_ERR_IJ_FORCE_DEBUG_INIT_FAIL, &ret);
	if (ret < 0) {
		printk(KERN_ERR "ps3stor log init fail\n");
		goto l_debug_init_failed;
	}

	ps3cmd_init();
	ps3_mgmt_info_init();

	LOG_FILE_WARN("ps3stor driver version[%s], commit_id[%s],"
		"build_time[%s], toolchain_id[%s]\n",
		PS3_DRV_VERSION, PS3_DRV_COMMIT_ID,
		PS3_DRV_BUILD_TIME, PS3_DRV_TOOLCHAIN_ID);

	ret = ps3_sas_attach_transport();
	INJECT_START(PS3_ERR_IJ_FORCE_TRANSPORT_INIT_FAIL, &ret);
	if (ret != PS3_SUCCESS) {
		printk(KERN_ERR "ps3stor transport fail\n");
		ret = -ENODEV;
		goto l_sas_transport_failed;
	}

	ps3_host_info_get();

	ret = register_chrdev(0, PS3_CHRDEV_NAME, &ps3_mgmt_fops);
	INJECT_START(PS3_ERR_IJ_FORCE_CHARDEV_INIT_FAIL, &ret);
	if (ret < 0) {
		LOG_ERROR("ps3stor: failed to open device node\n");
		goto l_register_chrdev_failed;
	}
	ps3_mgmt_chrdev_major_no = ret;

	init_pci_driver(&ps3_pci_driver);
	ret = pci_register_driver(&ps3_pci_driver);
	INJECT_START(PS3_ERR_IJ_FORCE_PCIDRV_INIT_FAIL, &ret);
	if (ret) {
		LOG_ERROR("ps3stor: PCI hotplug registration failed \n");
		goto l_pci_register_driver_failed;
	}

	ret = driver_create_file(&ps3_pci_driver.driver, &driver_attr_version);
	INJECT_START(PS3_ERR_IJ_FORCE_VERFILE_INIT_FAIL, &ret);
	if (ret) {
		goto l_attr_ver_failed;
	}
	ret = driver_create_file(&ps3_pci_driver.driver,
			&driver_attr_release_date);
	INJECT_START(PS3_ERR_IJ_FORCE_VERFILE2_INIT_FAIL, &ret);
	if (ret) {
		goto l_attr_release_date_failed;
	}

	ps3_trace_id_init();
	ps3_version_verbose_fill();

	return ret;

l_attr_release_date_failed:
	driver_remove_file(&ps3_pci_driver.driver, &driver_attr_version);
l_attr_ver_failed:
	pci_unregister_driver(&ps3_pci_driver);
l_pci_register_driver_failed:
	unregister_chrdev(ps3_mgmt_chrdev_major_no, PS3_CHRDEV_NAME);
l_register_chrdev_failed:
	ps3_sas_release_transport();
l_sas_transport_failed:
	ps3_debug_exit();
	ps3cmd_exit();
l_debug_init_failed:
	return ret;
}

static void __exit ps3stor_exit(void)
{
	driver_remove_file(&ps3_pci_driver.driver, &driver_attr_version);
	driver_remove_file(&ps3_pci_driver.driver, &driver_attr_release_date);

	pci_unregister_driver(&ps3_pci_driver);
	unregister_chrdev(ps3_mgmt_chrdev_major_no, PS3_CHRDEV_NAME);

	ps3_sas_release_transport();
	ps3_debug_exit();
	ps3cmd_exit();
	return;
}

MODULE_INFO(private_version, PS3_PRIVATE_VERSION);
MODULE_INFO(product_support, PS3_DRV_PRODUCT_SUPPORT);
MODULE_INFO(build_time, PS3_DRV_BUILD_TIME);
MODULE_INFO(toolchain_id, PS3_DRV_TOOLCHAIN_ID);
MODULE_INFO(commit_id, PS3_DRV_COMMIT_ID);
MODULE_VERSION(PS3_DRV_VERSION);
MODULE_AUTHOR(PS3_DRV_AUTHOR);
MODULE_DESCRIPTION(PS3_DRV_DESCRIPTION);
MODULE_LICENSE(PS3_DRV_LICENSE);

module_init(ps3stor_init);
module_exit(ps3stor_exit);
