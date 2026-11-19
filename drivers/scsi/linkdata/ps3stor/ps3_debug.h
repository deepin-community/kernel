/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_DEBUG_H_
#define _PS3_DEBUG_H_

#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>

struct ps3_reg_dump_attr {
	unsigned long long read_dump_timestamp;
	unsigned long long write_dump_timestamp;
	unsigned long long read_dump_interval_ms;
	unsigned long long write_dump_interval_ms;
	unsigned long long lastest_value;
	char name[32];
};

struct ps3_debug_context {
	unsigned char io_trace_switch;
	unsigned char reserved[7];
	struct ps3_reg_dump_attr
		reg_dump[PS3_REGISTER_SET_SIZE / sizeof(unsigned long long)];
	struct Ps3DebugMemEntry debug_mem_vaddr[PS3_DEBUG_MEM_ARRAY_MAX_NUM];
	struct Ps3DebugMemEntry *debug_mem_buf;
	dma_addr_t debug_mem_buf_phy;
	unsigned int debug_mem_array_num;
	unsigned char reserved1[4];
};

void ps3_debug_context_init(struct ps3_instance *instance);

void ps3_reg_dump(struct ps3_instance *instance, void __iomem *reg,
		  unsigned long long value, unsigned char is_read);

ssize_t ps3_vd_io_outstanding_show(struct device *cdev,
				   struct device_attribute *attr, char *buf);

ssize_t ps3_io_outstanding_show(struct device *cdev,
				struct device_attribute *attr, char *buf);

ssize_t ps3_is_load_show(struct device *cdev, struct device_attribute *attr,
			 char *buf);

ssize_t ps3_dump_ioc_regs_show(struct device *cdev,
			       struct device_attribute *attr, char *buf);

ssize_t ps3_max_scsi_cmds_show(struct device *cdev,
			       struct device_attribute *attr, char *buf);

ssize_t ps3_event_subscribe_info_show(struct device *cdev,
				      struct device_attribute *attr, char *buf);

ssize_t ps3_ioc_state_show(struct device *cdev, struct device_attribute *attr,
			   char *buf);

ssize_t ps3_log_level_store(struct device *cdev, struct device_attribute *attr,
			    const char *buf, size_t count);
ssize_t ps3_log_level_show(struct device *cdev, struct device_attribute *attr,
			   char *buf);

ssize_t ps3_io_trace_switch_store(struct device *cdev,
				  struct device_attribute *attr,
				  const char *buf, size_t count);

ssize_t ps3_io_trace_switch_show(struct device *cdev,
				 struct device_attribute *attr, char *buf);
ssize_t ps3_halt_support_cli_show(struct device *cdev,
				  struct device_attribute *attr, char *buf);
ssize_t ps3_halt_support_cli_store(struct device *cdev,
				   struct device_attribute *attr,
				   const char *buf, size_t count);

#if defined(PS3_SUPPORT_DEBUG) ||                                              \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) ||           \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
#else
ssize_t ps3_irq_prk_support_store(struct device *cdev,
				  struct device_attribute *attr,
				  const char *buf, size_t count);

ssize_t ps3_irq_prk_support_show(struct device *cdev,
				 struct device_attribute *attr, char *buf);

#endif

ssize_t ps3_product_model_show(struct device *cdev,
			       struct device_attribute *attr, char *buf);

ssize_t ps3_qos_switch_show(struct device *cdev, struct device_attribute *attr,
			    char *buf);

ssize_t ps3_qos_switch_store(struct device *cdev, struct device_attribute *attr,
			     const char *buf, size_t count);

#endif
ssize_t ps3_event_subscribe_info_get(struct ps3_instance *instance, char *buf,
				     ssize_t total_len);

#ifndef _WINDOWS

int ps3_debug_mem_alloc(struct ps3_instance *ins);
int ps3_debug_mem_free(struct ps3_instance *ins);
ssize_t ps3_dump_state_show(struct device *cdev, struct device_attribute *attr,
			    char *buf);
ssize_t ps3_dump_state_store(struct device *cdev, struct device_attribute *attr,
			     const char *buf, size_t count);
ssize_t ps3_dump_type_show(struct device *cdev, struct device_attribute *attr,
			   char *buf);

ssize_t ps3_dump_type_store(struct device *cdev, struct device_attribute *attr,
			    const char *buf, size_t count);

ssize_t ps3_dump_dir_show(struct device *cdev, struct device_attribute *attr,
			  char *buf);

void ps3_dma_dump_mapping(struct pci_dev *pdev);

ssize_t ps3_soc_dead_reset_store(struct device *cdev,
				 struct device_attribute *attr, const char *buf,
				 size_t count);

struct ps3_instance *ps3_debug_instance_query(struct device *cdev,
	struct device_attribute *attr, const char *buf);

#else

struct ps3_reg_dump_attr {
	unsigned long long read_dump_timestamp;
	unsigned long long write_dump_timestamp;
	unsigned long long read_dump_interval_ms;
	unsigned long long write_dump_interval_ms;
	unsigned long long lastest_value;
	char name[32];
};

struct ps3_debug_context {
	unsigned char io_trace_switch;
	unsigned char reserved[7];
	struct ps3_reg_dump_attr
		reg_dump[PS3_REGISTER_SET_SIZE / sizeof(unsigned long long)];
	struct Ps3DebugMemEntry debug_mem_vaddr[PS3_DEBUG_MEM_ARRAY_MAX_NUM];
	struct Ps3DebugMemEntry *debug_mem_buf;
	dma_addr_t debug_mem_buf_phy;
	unsigned int debug_mem_array_num;
	unsigned char reserved1[4];
};

void ps3_debug_context_init(struct ps3_instance *instance);

void ps3_reg_dump(struct ps3_instance *instance, void __iomem *reg,
		  unsigned long long value, unsigned char is_read);

#endif

#endif
