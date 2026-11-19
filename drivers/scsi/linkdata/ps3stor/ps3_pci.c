// SPDX-License-Identifier: GPL-2.0

#include "ps3_instance_manager.h"
#include "ps3_pci.h"

#define PS3_VALID_MEMORY_BAR_INDEX 2
#ifdef _WINDOWS
#define PS3_CHECK_BAR_64BIT(bar) ((bar) & 0x4)
#define PS3_CHECK_BAR_MEMORYSPACE(bar) (!((bar) & 0x1))
#define PS3_BAR_BASE_ADDR_MARSK (0xFFFFFFF0)
#define PS3_GET_BAR_BASE_ADDR(addr)                                            \
	((unsigned long long)((addr) & PS3_BAR_BASE_ADDR_MARSK))

static int ps3_pci_map_reg(struct ps3_instance *instance,
			   PPORT_CONFIGURATION_INFORMATION config);
static int ps3_pci_info_get(struct ps3_instance *instance,
			    PPORT_CONFIGURATION_INFORMATION config);
static void ps3_pci_unmap_reg(struct ps3_instance *instance);
static void ps3_pci_irq_type_get(struct ps3_instance *instance,
				 PPCI_COMMON_CONFIG pci_config);
static unsigned short ps3_pci_msix_vec_count(struct ps3_instance *instance,
					     PPCI_COMMON_CONFIG pci_config);
static unsigned short ps3_pci_msi_vec_count(struct ps3_instance *instance,
					    PPCI_COMMON_CONFIG pci_config);
int ps3_pci_init(struct ps3_instance *instance, void *config)
{
	int ret = PS3_SUCCESS;
	PPORT_CONFIGURATION_INFORMATION config_info =
		(PPORT_CONFIGURATION_INFORMATION)config;

	ret = ps3_pci_info_get(instance, config_info);
	if (ret != PS3_SUCCESS)
		goto l_out;

	ret = ps3_pci_map_reg(instance, config_info);
	if (ret != PS3_SUCCESS)
		goto l_out;

l_out:
	return ret;
}

void ps3_pci_exit(struct ps3_instance *instance)
{
	ps3_pci_unmap_reg(instance);
}

static int ps3_pci_info_get(struct ps3_instance *instance,
			    PPORT_CONFIGURATION_INFORMATION config)
{
	PCI_COMMON_CONFIG pci_config = { 0 };
	unsigned long len = 0;
	unsigned long base_addr = 0;

	if (config->AdapterInterfaceType != PCIBus) {
		LOG_ERROR("there is not pcibus, type:%d\n",
			  config->AdapterInterfaceType);
		return -PS3_FAILED;
	}

	len = StorPortGetBusData(instance, PCIConfiguration,
				 config->SystemIoBusNumber,
				 (unsigned long)config->SlotNumber,
				 (void *)&pci_config, sizeof(pci_config));

	if (len == 0 || len == 2) {
		LOG_ERROR("get bus data failed,cfg length:%d\n", len);
		return -PS3_FAILED;
	}

	instance->pci_dev_context.slot_number =
		(unsigned long long)config->SlotNumber;
	instance->pci_dev_context.device_id = pci_config.DeviceID;
	instance->pci_dev_context.vendor_id = pci_config.VendorID;
	instance->pci_dev_context.sub_vendor_id =
		pci_config.u.type0.SubVendorID;
	instance->pci_dev_context.sub_device_id =
		pci_config.u.type0.SubSystemID;
	base_addr =
		pci_config.u.type0.BaseAddresses[PS3_VALID_MEMORY_BAR_INDEX];
	if (!PS3_CHECK_BAR_MEMORYSPACE(base_addr)) {
		LOG_ERROR("Bar%d is not memory space\n",
			  PS3_VALID_MEMORY_BAR_INDEX);
		return -PS3_FAILED;
	}

	if (PS3_CHECK_BAR_64BIT(base_addr)) {
		instance->pci_dev_context.bar_base_addr =
			PS3_GET_BAR_BASE_ADDR(base_addr);
		base_addr =
			pci_config.u.type0
				.BaseAddresses[PS3_VALID_MEMORY_BAR_INDEX + 1];
		instance->pci_dev_context.bar_base_addr |=
			((unsigned long long)base_addr) << 32;
	} else {
		instance->pci_dev_context.bar_base_addr =
			PS3_GET_BAR_BASE_ADDR(base_addr);
	}

	ps3_pci_irq_type_get(instance, &pci_config);

	if (instance->pci_dev_context.pci_irq_type == PS3_PCI_IRQ_MSIX) {
		instance->pci_dev_context.irq_vec_count =
			ps3_pci_msix_vec_count(instance, &pci_config);
	} else if (instance->pci_dev_context.pci_irq_type == PS3_PCI_IRQ_MSI) {
		instance->pci_dev_context.irq_vec_count =
			ps3_pci_msi_vec_count(instance, &pci_config);
	} else {
		instance->pci_dev_context.irq_vec_count = 1;
	}

	LOG_INFO(
		"vid:%x, devid:%x, sub_vid:%x, sub_devid:%x, bar_base:0x%llx, irq_type:%d, vec_count:%d\n",
		instance->pci_dev_context.vendor_id,
		instance->pci_dev_context.device_id,
		instance->pci_dev_context.sub_vendor_id,
		instance->pci_dev_context.sub_device_id,
		instance->pci_dev_context.bar_base_addr,
		instance->pci_dev_context.pci_irq_type,
		instance->pci_dev_context.irq_vec_count);

	return PS3_SUCCESS;
};

static int ps3_pci_map_reg(struct ps3_instance *instance,
			   PPORT_CONFIGURATION_INFORMATION config)
{
	int ret = -PS3_FAILED;
	unsigned long i = 0;

	if (config->NumberOfAccessRanges <= 0) {
		LOG_ERROR("valid access range is 0\n");
		goto l_out;
	}

	if (config->AdapterInterfaceType != PCIBus) {
		LOG_ERROR("adapter interface type is not PCIBus\n");
		goto l_out;
	}

	for (i = 0; i < config->NumberOfAccessRanges; i++) {
		LOG_DEBUG("Bar%llu:0x%llx, memory:%d, len:0x%x, ex:0x%llx\n", i,
			  (*(config->AccessRanges))[i].RangeStart.QuadPart,
			  (*(config->AccessRanges))[i].RangeInMemory,
			  (*(config->AccessRanges))[i].RangeLength,
			  instance->pci_dev_context.bar_base_addr);
		if ((unsigned long long)(*(config->AccessRanges))[i]
			    .RangeStart.QuadPart !=
		    instance->pci_dev_context.bar_base_addr) {
			continue;
		}

		if (!(*(config->AccessRanges))[i].RangeInMemory) {
			LOG_INFO(
				"access range number continue:%d, is not memery\n",
				i);
			continue;
		}

		instance->reg_set = (struct Ps3Fifo *)StorPortGetDeviceBase(
			instance, config->AdapterInterfaceType,
			config->SystemIoBusNumber,
			(*(config->AccessRanges))[i].RangeStart,
			(*(config->AccessRanges))[i].RangeLength, 0);

		if (instance->reg_set != NULL)
			ret = PS3_SUCCESS;

		break;
	}

l_out:
	LOG_INFO("map reg:%d,reg:%p\n", ret, instance->reg_set);
	return ret;
}

static void ps3_pci_unmap_reg(struct ps3_instance *instance)
{
	if (instance->reg_set != NULL) {
		StorPortFreeDeviceBase(instance, instance->reg_set);
		instance->reg_set = NULL;
	}
}

static int __ps3_pci_find_capability(PPCI_COMMON_CONFIG pci_config, int cap_id)
{
	int pos = 0;
	unsigned char *config_base_addr = (unsigned char *)pci_config;
	unsigned char cap_offset = 0;
	PPCI_CAPABILITIES_HEADER cap_header = NULL;

	if (PCI_CONFIGURATION_TYPE(pci_config)) {
		LOG_ERROR("there is not agent device\n");
		goto l_out;
	}

	if ((pci_config->Status & PCI_STATUS_CAPABILITIES_LIST) == 0) {
		LOG_ERROR("capability pointer invalid\n");
		goto l_out;
	}

	cap_offset = pci_config->u.type0.CapabilitiesPtr;
	while (cap_offset != 0) {
		cap_header = (PPCI_CAPABILITIES_HEADER)(config_base_addr +
							cap_offset);
		if (cap_header->CapabilityID == 0) {
			cap_offset = cap_header->Next;
			continue;
		}

		if (cap_header->CapabilityID == (unsigned char)cap_id) {
			pos = cap_offset;
			break;
		}
		cap_offset = cap_header->Next;
	};

l_out:
	return pos;
}

static void ps3_pci_irq_type_get(struct ps3_instance *instance,
				 PPCI_COMMON_CONFIG pci_config)
{
	unsigned char *config_base_addr = (unsigned char *)pci_config;
	unsigned short data_u16 = 0;
	int pos = 0;

	instance->pci_dev_context.pci_irq_type = PS3_PCI_IRQ_LEGACY;
	pos = __ps3_pci_find_capability(pci_config, PCI_CAP_ID_MSIX);
	if (pos != 0) {
		data_u16 = *((unsigned short *)(config_base_addr + pos +
						PCI_MSIX_FLAGS));

		for (size_t i = 0; i < 4; i++) {
			unsigned char tmp = *(
				(unsigned char *)(config_base_addr + pos + i));

			LOG_DEBUG("%d:%d:%x\n", pos, i, tmp);
		}

		if ((data_u16 & PCI_MSIX_FLAGS_ENABLE) ==
		    PCI_MSIX_FLAGS_ENABLE) {
			instance->pci_dev_context.pci_irq_type =
				PS3_PCI_IRQ_MSIX;
			goto l_out;
		}
	}

	pos = __ps3_pci_find_capability(pci_config, PCI_CAP_ID_MSI);
	if (pos != 0) {
		data_u16 = *((unsigned short *)(config_base_addr + pos +
						PCI_MSI_FLAGS));
		if ((data_u16 & PCI_MSI_FLAGS_ENABLE) == PCI_MSI_FLAGS_ENABLE) {
			instance->pci_dev_context.pci_irq_type =
				PS3_PCI_IRQ_MSI;
			goto l_out;
		}
	}

l_out:
	return;
}

void ps3_pci_intx(struct ps3_instance *instance, unsigned char enable)
{
	unsigned short pci_command = 0;
	unsigned short pci_command_new = 0;

	if (ps3_pci_read_config_word(instance, PCI_COMMAND, &pci_command) !=
	    PS3_SUCCESS) {
		goto l_out;
	}

	if (enable)
		pci_command_new = pci_command & ~PCI_COMMAND_INTX_DISABLE;
	else
		pci_command_new = pci_command | PCI_COMMAND_INTX_DISABLE;

	if (pci_command_new != pci_command) {
		ps3_pci_write_config_word(instance, PCI_COMMAND,
					  pci_command_new);
	}

l_out:
	return;
}

static unsigned short ps3_pci_msix_vec_count(struct ps3_instance *instance,
					     PPCI_COMMON_CONFIG pci_config)
{
	unsigned char *config_base_addr = (unsigned char *)pci_config;
	int pos = 0;
	unsigned short msix_vec_count = 0;
	unsigned short data_u16 = 0;

	(void)instance;
	pos = __ps3_pci_find_capability(pci_config, PCI_CAP_ID_MSIX);
	if (pos != 0)
		data_u16 = *((unsigned short *)(config_base_addr + pos +
						PCI_MSIX_FLAGS));

	msix_vec_count = ((data_u16 & PCI_MSIX_FLAGS_QSIZE) + 1);

	return msix_vec_count;
}

static unsigned short ps3_pci_msi_vec_count(struct ps3_instance *instance,
					    PPCI_COMMON_CONFIG pci_config)
{
	unsigned char *config_base_addr = (unsigned char *)pci_config;
	int pos = 0;
	unsigned short msi_vec_count = 0;
	unsigned short data_u16 = 0;

	(void)instance;
	pos = __ps3_pci_find_capability(pci_config, PCI_CAP_ID_MSI);
	if (pos != 0)
		data_u16 = *((unsigned short *)(config_base_addr + pos +
						PCI_MSI_FLAGS));

	msi_vec_count = 1 << ((data_u16 & PCI_MSI_FLAGS_QMASK) >> 1);

	return data_u16;
}

#endif

int ps3_pci_find_capability(struct ps3_instance *instance, int cap_id)
{
	int pos = 0;
#ifdef _WINDOWS
	PCI_COMMON_CONFIG pci_config = { 0 };
	unsigned int len = 0;

	len = (unsigned int)StorPortGetBusData(
		instance, PCIConfiguration, (unsigned long)instance->bus_number,
		(unsigned long)instance->pci_dev_context.slot_number,
		(void *)&pci_config, (unsigned long)sizeof(pci_config));

	if (len == 0 || len == 2) {
		LOG_ERROR("get bus data failed,cfg length:%d\n", len);
		goto l_out;
	}

	pos = __ps3_pci_find_capability(&pci_config, cap_id);

l_out:
#else
	pos = pci_find_capability(instance->pdev, cap_id);
#endif
	return pos;
}

int ps3_pci_read_config_word(struct ps3_instance *instance, unsigned int offset,
			     unsigned short *val)
{
	int ret = -PS3_FAILED;
#ifdef _WINDOWS
	PCI_COMMON_CONFIG pci_config = { 0 };
	unsigned int len = 0;
	unsigned char *config_base_addr = (unsigned char *)&pci_config;

	len = (unsigned int)StorPortGetBusData(
		instance, PCIConfiguration, (unsigned long)instance->bus_number,
		(unsigned long)instance->pci_dev_context.slot_number,
		(void *)&pci_config, (unsigned long)sizeof(pci_config));

	if (len == 0 || len == 2) {
		LOG_ERROR("get bus data failed,cfg length:%d\n", len);
		goto l_out;
	}

	*val = *((unsigned short *)(config_base_addr + offset));
	ret = PS3_SUCCESS;

l_out:
#else
	ret = pci_read_config_word(instance->pdev, offset, val);
#endif
	LOG_INFO("read config word :%d\n", ret);
	return ret;
}

int ps3_pci_write_config_word(struct ps3_instance *instance,
			      unsigned int offset, unsigned short val)
{
	int ret = -PS3_FAILED;
#ifdef _WINDOWS
	unsigned int len = 0;

	len = (unsigned int)StorPortSetBusDataByOffset(
		instance, PCIConfiguration, (unsigned long)instance->bus_number,
		(unsigned long)instance->pci_dev_context.slot_number,
		(void *)&val, (unsigned long)offset,
		(unsigned long)sizeof(unsigned short));

	if (len == sizeof(unsigned short))
		ret = PS3_SUCCESS;
#else
	ret = pci_write_config_word(instance->pdev, offset, val);
#endif
	LOG_INFO("write config word :%d\n", ret);
	return ret;
}

void ps3_reg_write_u64(struct ps3_instance *instance, unsigned long long val,
		       void *reg)
{
#ifndef _WINDOWS
#if defined(writeq) && defined(CONFIG_64BIT)
	(void)instance;
	writeq(val, reg);
#else
	unsigned long flags;

	ps3_spin_lock_irqsave(&instance->req_queue_lock, &flags);
	writel((unsigned int)(val & 0xffffffff), reg);
	writel((unsigned int)(val >> 32), reg + 0x4UL);
	ps3_spin_unlock_irqrestore(&instance->req_queue_lock, flags);
#endif
#else
	StorPortWriteRegisterUlong64(instance, reg, val);
#endif
}

unsigned long long ps3_reg_read_u64(struct ps3_instance *instance, void *reg)
{
	unsigned long long value = 0;
	(void)instance;
#ifndef _WINDOWS
#if defined(readq) && defined(CONFIG_64BIT)
	value = readq(reg);
#else
	value = (((unsigned long long)readl(reg + 0x4UL) << 32) |
		 (unsigned long long)readl(reg));
#endif
#else

	value = StorPortReadRegisterUlong64(instance, reg);

#endif
	return value;
}

void __iomem *ps3_reg_set_ioremap(struct pci_dev *pdev, unsigned long reg_bar)
{
	resource_size_t base_addr = 0;

	base_addr = pci_resource_start(pdev, reg_bar);
	return ioremap(base_addr, PS3_REGISTER_SET_SIZE);
}
