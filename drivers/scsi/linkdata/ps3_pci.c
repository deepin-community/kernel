
#include "ps3_instance_manager.h"
#include "ps3_pci.h"

#define PS3_VALID_MEMORY_BAR_INDEX 2
#ifdef _WINDOWS
#define PS3_CHECK_BAR_64BIT(bar) ((bar)&0x4)
#define PS3_CHECK_BAR_MEMORYSPACE(bar) (!((bar)&0x1))
#define PS3_BAR_BASE_ADDR_MARSK	(0xFFFFFFF0)
#define PS3_GET_BAR_BASE_ADDR(addr) (U64)((addr) & PS3_BAR_BASE_ADDR_MARSK)

static S32 ps3_pci_map_reg(struct ps3_instance *instance, PPORT_CONFIGURATION_INFORMATION config);
static S32 ps3_pci_info_get(struct ps3_instance *instance, PPORT_CONFIGURATION_INFORMATION config);
static void ps3_pci_unmap_reg(struct ps3_instance *instance);
static void ps3_pci_irq_type_get(struct ps3_instance *instance, PPCI_COMMON_CONFIG pci_config);
static U16 ps3_pci_msix_vec_count(struct ps3_instance *instance, PPCI_COMMON_CONFIG pci_config);
static U16 ps3_pci_msi_vec_count(struct ps3_instance *instance, PPCI_COMMON_CONFIG pci_config);

S32 ps3_pci_init(struct ps3_instance *instance, void *config)
{
    S32 ret = PS3_SUCCESS;
    PPORT_CONFIGURATION_INFORMATION config_info = (PPORT_CONFIGURATION_INFORMATION)config;

    ret = ps3_pci_info_get(instance, config_info);
    if (ret != PS3_SUCCESS) {
        goto l_out;
    }

    ret = ps3_pci_map_reg(instance, config_info);
    if (ret != PS3_SUCCESS) {
        goto l_out;
    }

l_out:
    return ret;
}

void ps3_pci_exit(struct ps3_instance *instance)
{
    ps3_pci_unmap_reg(instance);
}

static S32 ps3_pci_info_get(
    struct ps3_instance *instance,
    PPORT_CONFIGURATION_INFORMATION config
)
{
    PCI_COMMON_CONFIG pci_config = { 0 };
    ULong len = 0;
    ULong base_addr = 0;
#ifdef PS3_HARDWARE_ASIC
	U32 check_count = ps3_hba_check_time_query() * 10;
#endif

    if (config->AdapterInterfaceType != PCIBus) {
        LOG_ERROR("there is not pcibus, type:%d\n", config->AdapterInterfaceType);
        return -PS3_FAILED;
    }

    len = StorPortGetBusData(instance,
        PCIConfiguration,
        config->SystemIoBusNumber,
        (ULong)config->SlotNumber,
        (void*)&pci_config,
        sizeof(pci_config));

    if (len == 0 || len == 2) {
        LOG_ERROR("get bus data failed,cfg length:%d\n", len);
        return -PS3_FAILED;
    }

#ifdef PS3_HARDWARE_ASIC
	while(pci_config.DeviceID == PCI_DEVICE_ID_PS3_RAID_FPGA &&
			check_count > 0) {

		check_count--;
		ps3_msleep(100);

		len = StorPortGetBusData(instance,
		        PCIConfiguration,
		        config->SystemIoBusNumber,
		        (ULong)config->SlotNumber,
		        (void*)&pci_config,
		        sizeof(pci_config));

		if (len == 0 || len == 2) {
			LOG_ERROR("get bus data failed,cfg length:%d\n", len);
			return -PS3_FAILED;
		}

		LOG_INFO("get real device id is[0x%x] \n", pci_config.DeviceID);
	};
#endif

    instance->pci_dev_context.slot_number = (U64)config->SlotNumber;
    instance->pci_dev_context.device_id = pci_config.DeviceID;
    instance->pci_dev_context.vendor_id = pci_config.VendorID;
    instance->pci_dev_context.sub_vendor_id = pci_config.u.type0.SubVendorID;
    instance->pci_dev_context.sub_device_id = pci_config.u.type0.SubSystemID;

    base_addr = pci_config.u.type0.BaseAddresses[PS3_VALID_MEMORY_BAR_INDEX];
    if (!PS3_CHECK_BAR_MEMORYSPACE(base_addr)) {
        LOG_ERROR("Bar%d is not memory space\n", PS3_VALID_MEMORY_BAR_INDEX);
        return -PS3_FAILED;
    }

    if (PS3_CHECK_BAR_64BIT(base_addr)) {
        instance->pci_dev_context.bar_base_addr =
            PS3_GET_BAR_BASE_ADDR(base_addr);
        base_addr = pci_config.u.type0.BaseAddresses[PS3_VALID_MEMORY_BAR_INDEX + 1];
        instance->pci_dev_context.bar_base_addr |= ((U64)base_addr) << 32;
    }
    else {
        instance->pci_dev_context.bar_base_addr =
            PS3_GET_BAR_BASE_ADDR(base_addr);
    }

    ps3_pci_irq_type_get(instance, &pci_config);

    if (instance->pci_dev_context.pci_irq_type == PS3_PCI_IRQ_MSIX) {
        instance->pci_dev_context.irq_vec_count = ps3_pci_msix_vec_count(instance, &pci_config);
    }
    else if (instance->pci_dev_context.pci_irq_type == PS3_PCI_IRQ_MSI) {
        instance->pci_dev_context.irq_vec_count = ps3_pci_msi_vec_count(instance, &pci_config);
    }
    else {
        instance->pci_dev_context.irq_vec_count = 1;
    }

    LOG_INFO("vid:%x, devid:%x, sub_vid:%x, sub_devid:%x, bar_base:0x%llx, irq_type:%d, vec_count:%d\n",
        instance->pci_dev_context.vendor_id,
        instance->pci_dev_context.device_id,
        instance->pci_dev_context.sub_vendor_id,
        instance->pci_dev_context.sub_device_id,
        instance->pci_dev_context.bar_base_addr,
        instance->pci_dev_context.pci_irq_type,
        instance->pci_dev_context.irq_vec_count);

    return PS3_SUCCESS;
};

static S32 ps3_pci_map_reg(struct ps3_instance *instance, PPORT_CONFIGURATION_INFORMATION config)
{
    S32 ret = -PS3_FAILED;
    ULong i = 0;

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
            (*(config->AccessRanges))[i].RangeStart.QuadPart, (*(config->AccessRanges))[i].RangeInMemory,
            (*(config->AccessRanges))[i].RangeLength,
            instance->pci_dev_context.bar_base_addr);
        if ((U64)(*(config->AccessRanges))[i].RangeStart.QuadPart != instance->pci_dev_context.bar_base_addr) {
            continue;
        }

        if (!(*(config->AccessRanges))[i].RangeInMemory) {
            LOG_INFO("access range number continue:%d, is not memery\n", i);
            continue;
        }

        instance->reg_set = (Ps3Fifo_s*)StorPortGetDeviceBase(instance,
            config->AdapterInterfaceType,
            config->SystemIoBusNumber,
            (*(config->AccessRanges))[i].RangeStart,
            (*(config->AccessRanges))[i].RangeLength,
            0);

        if (instance->reg_set != NULL) {
            ret = PS3_SUCCESS;
        }

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

static S32 __ps3_pci_find_capability(PPCI_COMMON_CONFIG pci_config, S32 cap_id)
{
    S32 pos = 0;
    U8 *config_base_addr = (U8*)pci_config;
    U8 cap_offset = 0;
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
        cap_header = (PPCI_CAPABILITIES_HEADER)(config_base_addr + cap_offset);
        if (cap_header->CapabilityID == 0) {
            cap_offset = cap_header->Next;
            continue;
        }

        if (cap_header->CapabilityID == (U8)cap_id) {
            pos = cap_offset;
            break;

        }
        cap_offset = cap_header->Next;
    };

l_out:
    return pos;
}

static void ps3_pci_irq_type_get(struct ps3_instance *instance, PPCI_COMMON_CONFIG pci_config)
{
    U8 *config_base_addr = (U8*)pci_config;
    U16 data_u16 = 0;
    S32 pos = 0;

    instance->pci_dev_context.pci_irq_type = PS3_PCI_IRQ_LEGACY;
    pos = __ps3_pci_find_capability(pci_config, PCI_CAP_ID_MSIX);
    if ( pos != 0) {
        data_u16 = *( (U16*)(config_base_addr + pos + PCI_MSIX_FLAGS) );

        for (size_t i = 0; i < 4; i++)
        {
            U8 tmp = *((U8*)(config_base_addr + pos + i));
            LOG_DEBUG("%d:%d:%x\n", pos, i, tmp);
        }

        if ((data_u16 & PCI_MSIX_FLAGS_ENABLE) == PCI_MSIX_FLAGS_ENABLE) {
            instance->pci_dev_context.pci_irq_type = PS3_PCI_IRQ_MSIX;
            goto l_out;
        }
    }

    pos = __ps3_pci_find_capability(pci_config, PCI_CAP_ID_MSI);
    if (pos != 0) {
        data_u16 = *((U16*)(config_base_addr + pos + PCI_MSI_FLAGS));
        if ( (data_u16 & PCI_MSI_FLAGS_ENABLE) == PCI_MSI_FLAGS_ENABLE) {
            instance->pci_dev_context.pci_irq_type = PS3_PCI_IRQ_MSI;
            goto l_out;
        }
    }

l_out:
    return;
}

void ps3_pci_intx(struct ps3_instance *instance, U8 enable)
{
    U16 pci_command = 0;
    U16 pci_command_new = 0;
    if (ps3_pci_read_config_word(instance, PCI_COMMAND, &pci_command) != PS3_SUCCESS) {
        goto l_out;
    }

    if (enable) {
        pci_command_new = pci_command & ~PCI_COMMAND_INTX_DISABLE;
    }
    else {
        pci_command_new = pci_command | PCI_COMMAND_INTX_DISABLE;
    }

    if (pci_command_new != pci_command) {
        ps3_pci_write_config_word(instance, PCI_COMMAND, pci_command_new);
    }

l_out:
    return;
}

static U16 ps3_pci_msix_vec_count(struct ps3_instance *instance, PPCI_COMMON_CONFIG pci_config)
{
    U8 *config_base_addr = (U8*)pci_config;
    S32 pos = 0;
    U16 msix_vec_count = 0;
    U16 data_u16 = 0;

    (void)instance;
    pos = __ps3_pci_find_capability(pci_config, PCI_CAP_ID_MSIX);
    if (pos != 0) {
        data_u16 = *((U16*)(config_base_addr + pos + PCI_MSIX_FLAGS));
    }

    msix_vec_count = ((data_u16 & PCI_MSIX_FLAGS_QSIZE) + 1);

    return msix_vec_count;
}

static U16 ps3_pci_msi_vec_count(struct ps3_instance *instance, PPCI_COMMON_CONFIG pci_config)
{
    U8 *config_base_addr = (U8*)pci_config;
    S32 pos = 0;
    U16 msi_vec_count = 0;
    U16 data_u16 = 0;

    (void)instance;
    pos = __ps3_pci_find_capability(pci_config, PCI_CAP_ID_MSI);
    if (pos != 0) {
        data_u16 = *((U16*)(config_base_addr + pos + PCI_MSI_FLAGS));
    }

    msi_vec_count = 1 << ((data_u16 & PCI_MSI_FLAGS_QMASK) >> 1);

    return data_u16;
}

#endif

S32 ps3_pci_find_capability(struct ps3_instance *instance, S32 cap_id)
{
	S32 pos = 0;
#ifdef _WINDOWS
    PCI_COMMON_CONFIG pci_config = { 0 };
    U32 len = 0;

    len = (U32)StorPortGetBusData(instance,
        PCIConfiguration,
        (ULong)instance->bus_number,
        (ULong)instance->pci_dev_context.slot_number,
        (void*)&pci_config,
        (ULong)sizeof(pci_config));

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

S32 ps3_pci_read_config_word(struct ps3_instance *instance, U32 offset, U16 *val)
{
    S32 ret = -PS3_FAILED;
#ifdef _WINDOWS
    PCI_COMMON_CONFIG pci_config = { 0 };
    U32 len = 0;
    U8 *config_base_addr = (U8*)&pci_config;

    len = (U32)StorPortGetBusData(instance,
        PCIConfiguration,
        (ULong)instance->bus_number,
        (ULong)instance->pci_dev_context.slot_number,
        (void*)&pci_config,
        (ULong)sizeof(pci_config));

    if (len == 0 || len == 2) {
        LOG_ERROR("get bus data failed,cfg length:%d\n", len);
        goto l_out;
    }

    *val = *( (U16*)(config_base_addr + offset) );
    ret = PS3_SUCCESS;

l_out:
#else
	ret = pci_read_config_word(instance->pdev, offset, val);
#endif
    LOG_INFO("read config word :%d\n", ret);
    return ret;
}

S32 ps3_pci_write_config_word(struct ps3_instance *instance, U32 offset, U16 val)
{
    S32 ret = -PS3_FAILED;
#ifdef _WINDOWS
    U32 len = 0;
    len = (U32)StorPortSetBusDataByOffset(instance,
        PCIConfiguration,
        (ULong)instance->bus_number,
        (ULong)instance->pci_dev_context.slot_number,
        (void*)&val,
        (ULong)offset,
        (ULong)sizeof(U16)
    );

    if (len == sizeof(U16)) {
        ret = PS3_SUCCESS;
    }
#else
	ret = pci_write_config_word(instance->pdev, offset,
		val);
#endif
    LOG_INFO("write config word :%d\n", ret);
    return ret;
}

void ps3_reg_write_u64(struct ps3_instance *instance, U64 val, void *reg)
{
    (void)instance;
#ifndef _WINDOWS
#if defined(writeq) && defined(CONFIG_64BIT)
    writeq(val, reg);
#else
    ULong flags;
    ps3_spin_lock_irqsave(&instance->req_queue_lock, &flags);
    writel((U32)(val & 0xffffffff), reg);
    writel((U32)(val >> 32), reg + 0x4UL);
    ps3_spin_unlock_irqrestore(&instance->req_queue_lock, flags);
#endif
#else
    StorPortWriteRegisterUlong64(instance, reg, val);
#endif

}

U64 ps3_reg_read_u64(struct ps3_instance *instance, void *reg)
{
    U64 value = 0;
    (void)instance;
#ifndef _WINDOWS
#if defined(readq) && defined(CONFIG_64BIT)
    value = readq(reg);
#else
    value = (((U64)readl(reg + 0x4UL) << 32) |
        (U64)readl(reg));
#endif
#else

    value = StorPortReadRegisterUlong64(instance, reg);

#endif
    return value;
}

void __iomem *ps3_reg_set_ioremap(struct pci_dev *pdev, ULong reg_bar)
{
	resource_size_t base_addr = 0;

	base_addr = pci_resource_start(pdev, reg_bar);
	return ioremap(base_addr,PS3_REGISTER_SET_SIZE);
}