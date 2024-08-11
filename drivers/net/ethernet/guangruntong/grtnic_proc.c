#include <linux/proc_fs.h>
//#include <linux/version.h>
#include "grtnic.h"
#ifdef GRTNIC_PROCFS

struct proc_dir_entry *grtnic_top_dir = NULL;

#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0) )
//ssize_t update_firmware(struct file *file, const char __user *buffer, size_t count, loff_t *pos);

//static int grtnic_driver_generic_read(struct seq_file *m, void *v)
//{
//	return 0;
//}

//static int grtnic_driver_generic_open(struct inode *inode, struct file *file)
//{
//	return single_open(file, NULL, PDE_DATA(inode));
//}

////////////////////////////////////////////////////////////////////////////////////////////////
static int grtnic_driver_pktcnt_read(struct seq_file *m, void *v)
{
	struct grtnic_adapter *adapter = (struct grtnic_adapter *)m->private;

	seq_printf(m, "tx_count0 = %u,  tx_count1 = %u, rx_count = %u\n", adapter->tx_count0, adapter->tx_count1, adapter->rx_count);
	return 0;
}

static int grtnic_driver_pktcnt_open(struct inode *inode, struct file *file)
{
	return single_open(file, grtnic_driver_pktcnt_read, PDE_DATA(inode));
}
////////////////////////////////////////////////////////////////////////////////////////////////
static int grtnic_hardware_pktcnt_read(struct seq_file *m, void *v)
{
	u32 h2c_cnt, c2h_cnt, rx_cnt;
	struct grtnic_adapter *adapter = (struct grtnic_adapter *)m->private;
	struct grtnic_hw *hw = &adapter->hw;

	h2c_cnt = readl(hw->user_bar + 0x604);
	c2h_cnt = readl(hw->user_bar + 0x608);
	rx_cnt  = readl(hw->user_bar + 0x60C);

	seq_printf(m, "h2c_count = %u, c2h_count = %u, rx_count = %u\n", h2c_cnt, c2h_cnt, rx_cnt);
	return 0;
}

static int grtnic_hardware_pktcnt_open(struct inode *inode, struct file *file)
{
	return single_open(file, grtnic_hardware_pktcnt_read, PDE_DATA(inode));
}
////////////////////////////////////////////////////////////////////////////////////////////////

static int grtnic_hardware_error(struct seq_file *m, void *v)
{
	u32 var;
	int read_count_error, sgtxfifo_error, sgrxfifo_error, mainfifo_error;
	struct grtnic_adapter *adapter = (struct grtnic_adapter *)m->private;
	struct grtnic_hw *hw = &adapter->hw;

	var = readl(hw->user_bar + 0x600);

	read_count_error = (var >> 31) & 0x01;
	sgtxfifo_error = (var >> 16) & 0xff;
	sgrxfifo_error = (var >> 8) & 0xff;
	mainfifo_error = (var >> 0) & 0xff;
	seq_printf(m, "read_count_error = %d,  sgtxfifo_error = %d, sgrxfifo_error = %d, mainfifo_error = %d\n", read_count_error, sgtxfifo_error, sgrxfifo_error, mainfifo_error);
	return 0;
}

static int grtnic_hardware_error_open(struct inode *inode, struct file *file)
{
	return single_open(file, grtnic_hardware_error, PDE_DATA(inode));
}

////////////////////////////////////////////////////////////////////////////////////////////////
struct grtnic_proc_type {
	char *name;
	int (*open)(struct inode *inode, struct file *file);
	int (*read)(struct seq_file *m, void *v);
	ssize_t (*write)(struct file *file, const char __user *buffer, size_t count, loff_t *pos);
};

struct grtnic_proc_type grtnic_proc_entries[] = {
	{"pktcnt", 		&grtnic_driver_pktcnt_open, NULL, NULL},
	{"fpktcnt", 	&grtnic_hardware_pktcnt_open, NULL, NULL},
	{"fharderr", 	&grtnic_hardware_error_open, NULL, NULL},
//	{"update", 		&grtnic_driver_generic_open, NULL, &update_firmware},
	{NULL, NULL, NULL, NULL}
};

////////////////////////////////////////////////////////////////////////////////////////////////


#else //LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0)
//int update_firmware(struct file *file, const char *buffer, unsigned long count, void *data);

static int grtnic_driver_pktcnt_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	struct grtnic_adapter *adapter = (struct grtnic_adapter *)data;

	return snprintf(page, count, "tx_count0 = %u,  tx_count1 = %u, rx_count = %d\n", adapter->tx_count0, adapter->tx_count1, adapter->rx_count);
}

static int grtnic_hardware_pktcnt_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	u32 h2c_cnt, c2h_cnt, rx_cnt;
	struct grtnic_adapter *adapter = (struct grtnic_adapter *)data;
	struct grtnic_hw *hw = &adapter->hw;

	h2c_cnt = readl(hw->user_bar + 0x604);
	c2h_cnt = readl(hw->user_bar + 0x608);
	rx_cnt  = readl(hw->user_bar + 0x60C);

	return snprintf(page, count, "h2c_count = %u, c2h_count = %u, rx_count = %u\n", h2c_cnt, c2h_cnt, rx_cnt);
}

static int grtnic_hardware_error(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	u32 var;
	int read_count_error, sgtxfifo_error, sgrxfifo_error, mainfifo_error;
	struct grtnic_adapter *adapter = (struct grtnic_adapter *)data;
	struct grtnic_hw *hw = &adapter->hw;

	var = readl(hw->user_bar + 0x600);

	read_count_error = (var >> 31) & 0x01;
	sgtxfifo_error = (var >> 16) & 0xff;
	sgrxfifo_error = (var >> 8) & 0xff;
	mainfifo_error = (var >> 0) & 0xff;

	return snprintf(page, count, "read_count_error = %d,  sgtxfifo_error = %d, sgrxfifo_error = %d, mainfifo_error = %d\n", read_count_error, sgtxfifo_error, sgrxfifo_error, mainfifo_error);
}

struct grtnic_proc_type {
	char *name;
	int (*read)(char *page, char **start, off_t off, int count, int *eof, void *data);
	int (*write)(struct file *file, const char *buffer, unsigned long count, void *data);
};

struct grtnic_proc_type grtnic_proc_entries[] = {
	{"pktcnt", &grtnic_driver_pktcnt_read, NULL},
	{"fpktcnt", &grtnic_hardware_pktcnt_read, NULL},
	{"fharderr", &grtnic_hardware_error, NULL},
//	{"update", NULL, &update_firmware},
	{NULL, NULL, NULL}
};

#endif


int grtnic_procfs_topdir_init()
{
	grtnic_top_dir = proc_mkdir("driver/grtnic", NULL);
	if (grtnic_top_dir == NULL)
		return -ENOMEM;

	return 0;
}

void grtnic_procfs_topdir_exit()
{
	remove_proc_entry("driver/grtnic", NULL);
}


int grtnic_procfs_init(struct grtnic_adapter *adapter)
{
	int index;
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0) )
	struct file_operations *fops;
#else
	struct proc_dir_entry *p;
	mode_t mode = 0;
#endif

	adapter->proc_dir = proc_mkdir(pci_name(adapter->pdev), grtnic_top_dir);

#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0) )
	for (index = 0; grtnic_proc_entries[index].name; index++) {
		fops = kmalloc(sizeof(struct file_operations), GFP_KERNEL);
		fops->open = grtnic_proc_entries[index].open;
		fops->read = seq_read;
		fops->write = grtnic_proc_entries[index].write;
		fops->llseek = seq_lseek;
		fops->release = single_release;
    proc_create_data(grtnic_proc_entries[index].name, 0644, adapter->proc_dir, fops, adapter);
	}

#else
	for (index = 0; grtnic_proc_entries[index].name; index++) {
		if (grtnic_proc_entries[index].read)
			mode = S_IFREG | S_IRUGO;
		if (grtnic_proc_entries[index].write)
			mode |= S_IFREG | S_IWUSR;

		p = create_proc_entry(grtnic_proc_entries[index].name, mode, adapter->proc_dir);
    p->read_proc = grtnic_proc_entries[index].read;
    p->write_proc = grtnic_proc_entries[index].write;
    p->data = adapter;
	}
#endif

	return 0;
}


void grtnic_del_proc_entries(struct grtnic_adapter *adapter)
{
	int index;

	if (grtnic_top_dir == NULL)
		return;

	for (index = 0; ; index++)
	{
		if(grtnic_proc_entries[index].name == NULL)
			break;
		remove_proc_entry(grtnic_proc_entries[index].name, adapter->proc_dir);
	}

	if (adapter->proc_dir != NULL)
		remove_proc_entry(pci_name(adapter->pdev), grtnic_top_dir);
}


void grtnic_procfs_exit(struct grtnic_adapter *adapter)
{
	grtnic_del_proc_entries(adapter);
}

#endif //GRTNIC_PROCFS
