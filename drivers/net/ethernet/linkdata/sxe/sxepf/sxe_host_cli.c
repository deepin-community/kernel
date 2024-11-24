
#include "sxe.h"
#include "sxe_log.h"
#include "sxe_host_cli.h"
#include "sxe_host_hdc.h"
#include "sxe_ioctl.h"

static dev_t        sxe_cdev_major;
static struct class *sxe_cdev_class;

static struct mutex sxe_minor_lock;
static DEFINE_IDR(sxe_minor_idr);

static s32 sxe_cli_open(struct inode *inode, struct file *filep)
{
	struct sxe_adapter   *adapter;

	adapter = container_of(inode->i_cdev,
			struct sxe_adapter, cdev_info.cdev);
	LOG_DEBUG_BDF("open char dev of adapter[%p]\n", adapter);
	filep->private_data = adapter;

	return 0;
}

STATIC s32 sxe_cli_user_input_param_check(u64 trace_id,
						u8 *in_data, u16 in_len,
						u8 *out_data,u16 out_len)
{
	s32 ret  = -EINVAL;

	if (in_data == NULL || out_data == NULL) {
		LOG_ERROR("trace_id=0x%llx cmd paramter invalid,"
				"in_data=%p, out_data=%p\n",
				trace_id,in_data, out_data);
		goto l_out;
	}

	if (in_len == 0 || out_len == 0 || in_len > HDC_CACHE_TOTAL_LEN
		|| out_len > HDC_CACHE_TOTAL_LEN) {
		LOG_ERROR("trace_id=0x%llx cmd paramter invalid, "
			"inLen=%d, outLen=%d\n",
			trace_id, in_len, out_len);
		goto l_out;
	}

	return 0;
l_out:
	return ret;
}

static s32 sxe_do_cli_cmd(struct sxe_hw *hw, unsigned long arg)
{
	s32 ret = -SXE_FAILED;
	u8 *in_data;
	u16 in_len;
	u8 *out_data;
	u16 out_len;
	u64 trace_id;
	struct sxe_driver_cmd cmd;
	struct sxe_adapter *adapter = hw->adapter;

	struct SxeIoctlSyncCmd __user *user_cmd =
					(struct SxeIoctlSyncCmd __user*)arg;

	struct SxeIoctlSyncCmd  *user_cmd_buf =
			kzalloc(sizeof(struct SxeIoctlSyncCmd), GFP_KERNEL);
	if (user_cmd_buf == NULL) {
		LOG_ERROR_BDF("kzalloc user_cmd_buf mem failed\n");
		ret = -ENOMEM;
		goto l_ret;
	}

	if (copy_from_user(user_cmd_buf, (void __user*)user_cmd,
			 sizeof(struct SxeIoctlSyncCmd))) {
		LOG_ERROR_BDF("hw[%p] , copy from user err\n",hw);
		ret = -EFAULT;
		goto l_free;
	}

	in_data = user_cmd_buf->inData;
	in_len = user_cmd_buf->inLen;
	out_data = user_cmd_buf->outData;
	out_len = user_cmd_buf->outLen;
	trace_id = user_cmd_buf->traceid;

	LOG_DEBUG_BDF("get user cmd: trace_id=0x%llx,"
			"in_data len=%u, out_data len=%u\n",
			trace_id, in_len, out_len);
	ret = sxe_cli_user_input_param_check(trace_id, in_data, in_len,
						out_data, out_len);
	if (ret) {
		goto l_free;
	}

	cmd.req      = in_data;
	cmd.req_len  = in_len;
	cmd.resp     = out_data;
	cmd.resp_len = out_len;
	cmd.trace_id = trace_id;
	cmd.opcode   = SXE_CMD_MAX;
	cmd.is_interruptible = true;
	ret = sxe_cli_cmd_trans(hw, &cmd);
	if (ret) {
		LOG_ERROR_BDF("sxe cli cmd trace_id=0x%llx"
				"trans error, ret=%d\n", trace_id, ret);
		goto l_free;
	}

l_free:
	kfree(user_cmd_buf);
	user_cmd_buf = NULL;
l_ret:
	return ret;
}

static long sxe_cli_ioctl(struct file *filep,
				unsigned int cmd, unsigned long arg)
{
	long ret = -ENOTTY;
	struct sxe_hw *hw;
	struct sxe_adapter *adapter;

	if (filep == NULL || cmd == 0 || arg == 0) {
		LOG_ERROR("filep=%p cmd=%d arg=%ld\n", filep, cmd, arg);
		ret = -EINVAL;
		goto l_ioctl_failed;
	}

	adapter = (struct sxe_adapter *)filep->private_data;

	LOG_DEBUG_BDF("driver ioctl cmd=%x, arg=0x%lx\n", cmd, arg);

	if (adapter != NULL) {
		switch(cmd)
		{
		case SXE_CMD_IOCTL_SYNC_CMD:
			hw = &adapter->hw;
			ret = sxe_do_cli_cmd(hw, arg);
			break;
		default:
			LOG_ERROR_BDF("unknown ioctl cmd, filep=%p, cmd=%d,"
				"arg=0x%8.8lx\n", filep, cmd, arg);
			break;
		}
	} else {
		LOG_WARN_BDF("can found cdev\n");
		ret = -ENODEV;
		goto l_ioctl_failed;
	}

	if(ret) {
		LOG_ERROR_BDF("filp=%p, cmd=%d, arg=%lx, ret=%ld\n",
			filep, cmd, arg, ret);
		goto l_ioctl_failed;
	}

	return SXE_SUCCESS;

l_ioctl_failed:
	return ret;
}

const struct file_operations sxe_cdev_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = sxe_cli_ioctl,
	.open = sxe_cli_open,
	.release = NULL,
};

STATIC void sxe_pci_addr_get(struct pci_dev       *pci_dev,
					struct sxe_pci_addr *pci_addr)
{
	pci_addr->domain = pci_domain_nr(pci_dev->bus);
	pci_addr->bus = pci_dev->bus->number;
	pci_addr->deviceNo = (((pci_dev->devfn) >> PCI_BDF_DEV_SHIFT ) & PCI_BDF_DEV_MASK);
	pci_addr->devfn = ((pci_dev->devfn) & PCI_BDF_FUNC_MASK);

	return;
}

s32 sxe_cli_cdev_register(void)
{
	s32 ret;

	ret = alloc_chrdev_region(&sxe_cdev_major, 0,
				SXE_MAX_DEVICES_NUM, SXE_CHRDEV_NAME);
	if (ret) {
		LOG_ERROR("alloc cdev number failed\n");
		goto l_alloc_cdev_failed;
	}

#ifdef CLASS_CREATE_NEED_1_PARAM
	sxe_cdev_class = class_create(SXE_CHRDEV_CLASS_NAME);
#else
	sxe_cdev_class = class_create(THIS_MODULE, SXE_CHRDEV_CLASS_NAME);
#endif
	if(IS_ERR(sxe_cdev_class)) {
		ret = PTR_ERR(sxe_cdev_class);
		LOG_ERROR("create cdev class failed\n");
		goto l_create_class_failed;
	}

	mutex_init(&sxe_minor_lock);

	return SXE_SUCCESS;

l_create_class_failed:
	unregister_chrdev_region(sxe_cdev_major, SXE_MAX_DEVICES_NUM);
l_alloc_cdev_failed:
	return ret;

}

void sxe_cli_cdev_unregister(void)
{
	class_destroy (sxe_cdev_class);
	unregister_chrdev_region(sxe_cdev_major, SXE_MAX_DEVICES_NUM);
	idr_destroy(&sxe_minor_idr);

	return;
}

STATIC s32 sxe_get_minor(s32* dev_minor)
{
	s32 ret = -ENOMEM;

	mutex_lock(&sxe_minor_lock);
	ret = idr_alloc(&sxe_minor_idr, NULL, 0, SXE_MAX_DEVICES_NUM, GFP_KERNEL);
	if (ret >= 0) {
		*dev_minor = ret;
		ret = 0;
	}
	mutex_unlock(&sxe_minor_lock);
	return ret;
}

STATIC void sxe_free_minor(s32 dev_minor)
{
	mutex_lock(&sxe_minor_lock);
	idr_remove(&sxe_minor_idr, dev_minor);
	mutex_unlock(&sxe_minor_lock);
}

s32 sxe_cli_cdev_create(struct sxe_adapter *adapter)
{
	s32     ret;
	s32     dev_major, dev_minor;
	struct sxe_pci_addr pci_addr;

	ret = sxe_get_minor(&dev_minor);
	if	(ret)	{
		LOG_ERROR("cdev minor get failed, ret=%d\n", ret);
		ret = -ENOMEM;
		goto l_get_minor_failed;
	}

	dev_major = MAJOR(sxe_cdev_major);
	adapter->cdev_info.dev_no = MKDEV(dev_major, dev_minor);
	cdev_init(&adapter->cdev_info.cdev, &sxe_cdev_fops);
	adapter->cdev_info.cdev.owner = THIS_MODULE;
	adapter->cdev_info.cdev.ops = &sxe_cdev_fops;

	ret = cdev_add(&adapter->cdev_info.cdev, adapter->cdev_info.dev_no, 1);
	if (ret) {
		LOG_ERROR_BDF("failed to add cdev dev_no=%ld\n",
				(unsigned long)adapter->cdev_info.dev_no);
		goto l_add_cdev_failed;
	}

	sxe_pci_addr_get(adapter->pdev, &pci_addr);

	adapter->cdev_info.device = device_create(sxe_cdev_class, NULL,
				adapter->cdev_info.dev_no, NULL,
				SXE_CHRDEV_NAME "-%04x:%02x:%02x.%x", pci_addr.domain, pci_addr.bus,
				pci_addr.deviceNo, pci_addr.devfn);
	if (IS_ERR(adapter->cdev_info.device)) {
		ret = PTR_ERR(adapter->cdev_info.device);
		LOG_ERROR_BDF("failed to create device, dev_no=%ld\n",
				(unsigned long)adapter->cdev_info.dev_no);
		goto l_create_dev_failed;
	}

	LOG_INFO("create char dev[%p] dev_no[major:minor=%u:%u] on pci_dev[%p]"
		" to net_dev[%p] belongs to class dev[%p] success\n",
		&adapter->cdev_info.cdev, dev_major, dev_minor,
		adapter->pdev, adapter->netdev, adapter->cdev_info.device);

	return SXE_SUCCESS;

l_create_dev_failed:
	cdev_del(&adapter->cdev_info.cdev);
l_add_cdev_failed:
	sxe_free_minor(dev_minor);
l_get_minor_failed:
    return ret;
}

void sxe_cli_cdev_delete(struct sxe_adapter *adapter)
{
	s32	dev_minor;

	dev_minor = MINOR(adapter->cdev_info.dev_no);
	sxe_free_minor(dev_minor);

	LOG_INFO("delete char dev[%p], dev_no[major:minor=%u:%u]\n", 
		&adapter->cdev_info.cdev, MAJOR(adapter->cdev_info.dev_no), dev_minor);
	
	device_destroy(sxe_cdev_class, adapter->cdev_info.dev_no);
	cdev_del(&adapter->cdev_info.cdev);

	return;
}
