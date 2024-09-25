
#ifndef _PS3_DEBUG_H_
#define _PS3_DEBUG_H_

#ifndef  _WINDOWS
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>

struct ps3_reg_dump_attr {
	U64 	read_dump_timestamp;
	U64 	write_dump_timestamp;
	U64 	read_dump_interval_ms;
	U64 	write_dump_interval_ms;
	U64	lastest_value; 
	S8	name[32];   
};

struct ps3_debug_context {
	U8 io_trace_switch; 
	U8 reserved[7];
	struct ps3_reg_dump_attr reg_dump[PS3_REGISTER_SET_SIZE/sizeof(U64)];
	Ps3DebugMemEntry_s debug_mem_vaddr[PS3_DEBUG_MEM_ARRAY_MAX_NUM];
	Ps3DebugMemEntry_s *debug_mem_buf; 
	dma_addr_t debug_mem_buf_phy;   
	U32 debug_mem_array_num;  
	U8 reserved1[4];
};

void ps3_debug_context_init(struct ps3_instance *instance);

void ps3_reg_dump(struct ps3_instance *instance,
	void __iomem *reg, U64 value, Bool is_read);

ssize_t ps3_vd_io_outstanding_show(struct device *cdev,
	struct device_attribute *attr, char *buf);

ssize_t ps3_io_outstanding_show(struct device *cdev,
	struct device_attribute *attr, char *buf);

ssize_t ps3_is_load_show(struct device *cdev,
	struct device_attribute *attr, char *buf);

ssize_t ps3_dump_ioc_regs_show(struct device *cdev,
	struct device_attribute *attr, char *buf);

ssize_t ps3_max_scsi_cmds_show(struct device *cdev,
	struct device_attribute *attr, char *buf);

ssize_t ps3_event_subscribe_info_show(struct device *cdev,
	struct device_attribute *attr, char *buf);

ssize_t ps3_ioc_state_show(struct device *cdev, struct device_attribute *attr,
	char *buf);

ssize_t ps3_log_level_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count);

ssize_t ps3_log_level_show(struct device *cdev,
	struct device_attribute *attr, char *buf);

ssize_t ps3_io_trace_switch_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count);

ssize_t ps3_io_trace_switch_show(struct device *cdev,
	struct device_attribute *attr, char *buf);
ssize_t ps3_halt_support_cli_show(struct device *cdev,struct device_attribute *attr, char *buf);

ssize_t ps3_halt_support_cli_store(struct device *cdev,struct device_attribute *attr,
	const char *buf, size_t count);

void ps3_r1x_interval_delay_set(u32 r1x_interval_delay);

U32 ps3_r1x_interval_delay_get(void);

#if defined(PS3_SUPPORT_DEBUG) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_DBGBUG)) || \
	(defined(PS3_CFG_RELEASE) && defined(PS3_CFG_OCM_RELEASE))
#else
ssize_t ps3_irq_prk_support_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count);

ssize_t ps3_irq_prk_support_show(struct device *cdev,
	struct device_attribute *attr, char *buf);

#endif

ssize_t ps3_product_model_show(struct device *cdev,struct device_attribute *attr, char *buf);

ssize_t ps3_qos_switch_show(struct device *cdev,
	struct device_attribute *attr, char *buf);

ssize_t ps3_qos_switch_store(struct device *cdev,
	struct device_attribute *attr, const char *buf, size_t count);

#endif
ssize_t ps3_event_subscribe_info_get(struct ps3_instance *instance, char *buf,
	ssize_t total_len);

#ifndef _WINDOWS

S32 ps3_debug_mem_alloc(struct ps3_instance *ins);

S32 ps3_debug_mem_free(struct ps3_instance *ins);

ssize_t ps3_dump_state_show(struct device *cdev,struct device_attribute *attr, char *buf);

ssize_t ps3_dump_state_store(struct device *cdev,struct device_attribute *attr,
	const char *buf, size_t count);

ssize_t ps3_dump_type_show(struct device *cdev,struct device_attribute *attr, char *buf);

ssize_t ps3_dump_type_store(struct device *cdev, struct device_attribute *attr,
	const char *buf, size_t count);

#if 0
ssize_t ps3_dump_dir_store(struct device *cdev, struct device_attribute *attr,
	const char *buf, size_t count);
#endif
ssize_t ps3_dump_dir_show(struct device *cdev,struct device_attribute *attr, char *buf);

void ps3_dma_dump_mapping(struct pci_dev *pdev);

ssize_t ps3_soc_dead_reset_store(struct device *cdev, struct device_attribute *attr,
	const char *buf, size_t count);
#else

struct ps3_reg_dump_attr {
	U64 	read_dump_timestamp;
	U64 	write_dump_timestamp;
	U64 	read_dump_interval_ms;
	U64 	write_dump_interval_ms;
	U64	lastest_value; 
	S8	name[32];   
};

struct ps3_debug_context {
	U8 io_trace_switch; 
	U8 reserved[7];
	struct ps3_reg_dump_attr reg_dump[PS3_REGISTER_SET_SIZE / sizeof(U64)];
	Ps3DebugMemEntry_s debug_mem_vaddr[PS3_DEBUG_MEM_ARRAY_MAX_NUM];
	Ps3DebugMemEntry_s* debug_mem_buf; 
	dma_addr_t debug_mem_buf_phy;   
	U32 debug_mem_array_num;  
	U8 reserved1[4];
};

void ps3_debug_context_init(struct ps3_instance *instance);

void ps3_reg_dump(struct ps3_instance *instance,
	void __iomem* reg, U64 value, Bool is_read);

#endif

#endif
