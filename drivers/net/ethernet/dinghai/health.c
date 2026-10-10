#include <linux/dinghai/driver.h>
#include <linux/random.h>
#include "en_pf.h"
#include <linux/time.h>
#include "msg_common.h"
#include "en_pf/en_pf_eq.h"
#include "en_np/init/include/dpp_np_init.h"
#include "pcie_common.h"
#include "slib.h"

#define ZXDH_RISCV_HB_OFFSET      0x5300
#define ZXDH_M7_HB_OFFSET         0x5350
#define ZXDH_M7_ZIOS_LOG_OFFSET   0x10b000
#define ZXDH_M7_CGEL_LOG_OFFSET   0x3e0000
#define ZXDH_RISCV_FWLOG_OFFSET   0x100000
#define ZXDH_M7_LOG_SIZE          0x4010
#define ZXDH_ZIOS_LOG_SIZE        0x700000
#define ZXDH_CGEL_LOG_SIZE        0x120000
#define ZXDH_CGEL_ZIOS_SIZE       0x2000000
#define ZXDH_CHUNK_SIZE           0x100000

#define ZXDH_FOUR_BYTE_FF         0xffffffff


static void zxdh_start_health_poll(struct dh_core_dev *dh_dev);

#ifdef NEED_SYSFS_EMIT
int sysfs_emit(char *buf, const char *fmt, ...);
#endif

enum {
	ZXDH_HEALTH_POLL_INTERVAL = 1 * HZ,
	M7_LOGDUMP = 6,
	RISCV_LOG_DUMP = 10,
	M7_MAX_MISSES = 20,
	RISCV_BBX_DUMP = 40,
	RISCV_MAX_MISSES = 60,
};

#define INVALID_SYND 0xff
enum {
	RISCV_FW_EXCEPTION,
	RISCV_CORE_EXCEPTION,
	RISCV_COUNTER_MISSED,
	BAR_ERROR,
	VQM_FATAL,
	BTTL_FATAL,
	DRR_FATAL,
	OCM_FATAL,
	PCIE_FATAL,
	RDMA_FATAL,
	DTP_NP_RX_FATAL,
	DTP_VQM_RX_FATAL,
	DTP_NP_TX_FATAL,
	DTP_VQM_TX_FATAL,
	FLR_RESET,
	RISCV_SYND_COUNT_MAX,
	M7_COUNTER_MISSED = 32,
	SYND_COUNT_MAX,
};

static const char* synd_name[] = {
	"RISCV_FW_EXCEPTION",
	"RISCV_CORE_EXCEPTION",
	"RISCV_COUNTER_MISSED",
	"BAR_ERROR",
	"VQM_FATAL",
	"BTTL_FATAL",
	"DRR_FATAL",
	"OCM_FATAL",
	"PCIE_FATAL",
	"RDMA_FATAL",
	"DTP_NP_RX_FATAL",
	"DTP_VQM_RX_FATAL",
	"DTP_NP_TX_FATAL",
	"DTP_VQM_TX_FATAL",
	"FLR_RESET",
	"M7_COUNTER_MISSED",
};

static void dh_health_version_get(struct zxdh_core_health *health)
{
	struct health_buffer __iomem *hb = health->riscv.hb;
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);

	health->health_version = ioread8(&hb->health_version);
	HEAL_DEBUG_DEV(dh_dev, "%s health_version: %d\n", pci_name(dh_dev->pdev), health->health_version);
}

static void dh_health_version_update(struct zxdh_core_health *health)
{
	struct health_buffer __iomem *hb = health->riscv.hb;
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);

	uint8_t health_verison = 0;

	health_verison = ioread8(&hb->health_version);
	if ((health_verison > 1) || (health_verison == health->health_version))
		return;

	HEAL_INFO_DEV(dh_dev, "%s health_version changed from %d to %d\n", pci_name(dh_dev->pdev), health->health_version, health_verison);
	health->health_version = health_verison;
}

struct health_attribute {
	const char	*name;
	umode_t		mode;
	ssize_t (*store)(struct kobject *, struct kobj_attribute *, const char *, size_t);
};

static ssize_t health_action_store(struct kobject *kobj,
			struct kobj_attribute *attr, const char *buf, size_t count);

struct health_attribute health_attrs[DH_HEALTH_ATTR_NUM] = {
	{"fatal",			0440,	NULL},
	{"synd",			0440,	NULL},
	{"recovery_cnt",	0440,	NULL},
	{"action",			0640,	health_action_store},
	{"pstate_change_cnt",     0444,   NULL},
};

static ssize_t health_attrs_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	struct zxdh_core_health *health;
	int i = 0;

	for (i = 0; i < DH_HEALTH_ATTR_NUM; ++i) {
		if (strcmp(attr->attr.name, health_attrs[i].name) == 0) {
			break;
		}
	}

	HEAL_DEBUG("attr->attr.name = %s, i = %d\n", attr->attr.name, i);
	if (i == DH_HEALTH_ATTR_NUM)
		return -1;

	health = container_of(attr, struct zxdh_core_health, attrs[i]);

	switch(i) {
		case 0:
			return sysfs_emit(buf, "%d\n", health->fatal == 0 ? 0 : 1);
		case 1:
			return sysfs_emit(buf, "0x%lx\n", health->synd);
		case 2:
			return sysfs_emit(buf, "%d\n", health->recovery_cnt);
		case 3:
			return sysfs_emit(buf, "[%d] act_health_info_show, \
				[%d] act_bbx_log_dump, \
				[%d] act_reset, \
				[%d] act_reload \n",
				act_health_info_show,
				act_bbx_log_dump,
				act_reset,
				act_reload);
		case 4:
			return sysfs_emit(buf, "%d\n", health->pstate_change_cnt);
	}

	return -1;
}

static void get_m7_and_riscv_counter(struct zxdh_core_health *dh_health)
{
	struct health_buffer __iomem *riscv_hb = dh_health->riscv.hb;
	struct health_buffer __iomem *m7_hb = dh_health->m7.hb;
	uint32_t riscv_count;
	uint32_t m7_count;

	m7_count = ioread32(&m7_hb->health_counter);
	riscv_count = ioread32(&riscv_hb->health_counter);
	LOG_INFO("** m7_health_counter: 0x%x\n", m7_count);
	LOG_INFO("** riscv_health_counter: 0x%x\n", riscv_count);
}

/* Started by AICoder, pid:93afa285f226588143eb082bf008d94e9af6bb81 */
static void zxdh_health_info_show(struct dh_core_dev *dh_dev)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
    struct zxdh_core_health *health = &pf_dev->health;
    uint8_t i = 0;

    LOG_INFO("***************** %s nic info *****************\n", pci_name(dh_dev->pdev));
    LOG_INFO("** board_type: %d\n", pf_dev->board_type);
    LOG_INFO("** health_version: %d\n", health->health_version);
    LOG_INFO("** health_supported: %d\n", health->health_supported);
    if (!health->health_supported)
        return;
    LOG_INFO("** bar_chan_valid: %d\n", pf_dev->bar_chan_valid);
    LOG_INFO("** fast_unload: %d\n", pf_dev->fast_unload);
    LOG_INFO("** fatal: %d\n", health->fatal);
    LOG_INFO("** health->flags: %ld, 1 means ZXDH_DROP_NEW_HEALTH_WORK\n", health->flags);
    LOG_INFO("** recovery_cnt: %d\n", health->recovery_cnt);
    get_m7_and_riscv_counter(health);

    LOG_INFO("****************** health config ***************\n");
    LOG_INFO("** DH_HEALTH_ATTR_NUM: %d\n", DH_HEALTH_ATTR_NUM);
    LOG_INFO("** health->riscv.hb: 0x%p, health->m7.hb: 0x%p\n",
             health->riscv.hb, health->m7.hb);
    LOG_INFO("** m7_log_offset: 0x%llx, riscv_crdump_size: 0x%llx\n",
             health->m7_log_offset, health->riscv_crdump_size);
    LOG_INFO("** timer_poll: %d\n", ZXDH_HEALTH_POLL_INTERVAL);
    LOG_INFO("** M7_MAX_MISSES: %d\n", M7_MAX_MISSES);
    LOG_INFO("** RISCV_LOG_DUMP: %d\n", RISCV_LOG_DUMP);
    LOG_INFO("** RISCV_MAX_MISSES: %d\n", RISCV_MAX_MISSES);

    LOG_INFO("************ dh_dev->device_state: 0x%x **********\n", dh_dev->device_state);
    LOG_INFO("** [%d]: ZXDH_DEVICE_STATE_UNINITIALIZED\n", ZXDH_DEVICE_STATE_UNINITIALIZED);
    LOG_INFO("** [%d]: ZXDH_DEVICE_STATE_UP\n", ZXDH_DEVICE_STATE_UP);
    LOG_INFO("** [%d]: ZXDH_DEVICE_STATE_INTERNAL_ERROR\n", ZXDH_DEVICE_STATE_INTERNAL_ERROR);

    LOG_INFO("******************** synd: 0x%lx *************\n", health->synd);
    for (i = 0; i < SYND_COUNT_MAX; ++i) {
        if (i < RISCV_SYND_COUNT_MAX) {
            LOG_INFO("** bit[%d]: %s set %d times\n", i, synd_name[i], health->synd_statics[i]);
        } else if (i >= M7_COUNTER_MISSED) {
            LOG_INFO("** bit[%d]: %s set %d times\n", i, synd_name[i - 32 + RISCV_SYND_COUNT_MAX], health->synd_statics[i]);
        }
    }
    LOG_INFO("****************************************************\n");
}
/* Ended by AICoder, pid:93afa285f226588143eb082bf008d94e9af6bb81 */

static int32_t zxdh_pf_dh_reset_request(struct dh_core_dev *dh_dev);
static ssize_t health_action_store(struct kobject *kobj,
			struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct zxdh_core_health *health = container_of(attr, struct zxdh_core_health, attrs[3]);
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
	int err = 0;
	int action;

	err = kstrtoint(buf, 10, &action);
	if (err)
		return err;

	HEAL_DEBUG_DEV(dh_dev, "action = %d\n", action);
	switch(action) {
		case act_health_info_show:
			zxdh_health_info_show(dh_dev);
			break;
		case act_bbx_log_dump:
			queue_work(health->wq, &health->riscv_log_saving_work);
			queue_work(health->wq, &health->riscv_bbx_saving_work);
			queue_work(health->wq, &health->m7_bbx_saving_work);
			break;
		case act_reset:
			zxdh_pf_dh_reset_request(dh_dev);
			break;
		case act_reload:
			if (!zxdh_load_one(dh_dev))
				zxdh_start_health_poll(dh_dev);
			break;
	}

	return count;
}

static int zxdh_health_attr_create(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;
	struct kobj_attribute *attr = NULL;
	int err = 0;
	int i = 0;
	int j = 0;

	for (i = 0; i < DH_HEALTH_ATTR_NUM; ++i) {
		attr = &health->attrs[i];
		attr->attr.name = health_attrs[i].name;
		attr->attr.mode = health_attrs[i].mode;
		attr->show = health_attrs_show;
		attr->store = health_attrs[i].store;
		err = sysfs_create_file(&dh_dev->device->kobj, &attr->attr);
		if (err != 0) {
			HEAL_ERR_DEV(dh_dev, "%s %s sysfs_create_file failed!\n", pci_name(dh_dev->pdev), health_attrs[i].name);
			goto cleanup;
		}
	}

	return 0;

cleanup:
	for (j = --i; j >= 0; --j) {
		attr = &health->attrs[j];
		sysfs_remove_file(&dh_dev->device->kobj, &attr->attr);
	}
	return err;
}

static void zxdh_health_attr_remove(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;
	struct kobj_attribute *attr = NULL;
	int i = 0;

	for (i = 0; i < DH_HEALTH_ATTR_NUM; ++i) {
		attr = &health->attrs[i];
		sysfs_remove_file(&dh_dev->device->kobj, &attr->attr);
	}
}

struct bbox_hdr __iomem
{
	uint32_t magic;
	uint16_t start_offset;
	uint16_t end_offset;
	bool wrap;
	uint8_t rsv[3];
};

enum {
	FW_DEFAULT_TYPE,
	FW_CGEL_TYPE,
	FW_ZIOS_TYPE,
	FW_CGEL_TO_ZIOS_TYPE,
};

enum {
	ZIOS_M7_LOG,
	CGEL_M7_LOG,
	ZIOS_RISCV_LOG1,
	ZIOS_RISCV_LOG2,
	CGEL_RISCV_LOG1,
	CGEL_RISCV_LOG2,
};

static uint8_t log_name[6][32] = {
	"zios_m7_log",
	"cgel_m7_log",
	"zios_riscv_log1",
	"zios_riscv_log2",
	"cgel_riscv_log1",
	"cgel_riscv_log2",
};

void fw_core_dump_file(struct zxdh_pf_device *pf_dev, struct file *file, uint8_t* buf, uint64_t dump_size)
{
	loff_t pos = 0;
	int32_t ret = 0;
	void __iomem *src = (void __iomem *)(pf_dev->pci_ioremap_addr[2] + ZXDH_CGEL_LOG_SIZE);
	size_t remaining = dump_size - ZXDH_CGEL_LOG_SIZE;
	struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

	memcpy_fromio(buf, (void __iomem*)(pf_dev->pci_ioremap_addr[0] + ZXDH_RISCV_FWLOG_OFFSET), ZXDH_CGEL_LOG_SIZE);
#ifdef CGS_V5_693
	ret = kernel_write(file, buf, ZXDH_CGEL_LOG_SIZE, pos);
#else
	ret = kernel_write(file, buf, ZXDH_CGEL_LOG_SIZE, &pos);
#endif
	if (ret != ZXDH_CGEL_LOG_SIZE) {
		HEAL_ERR_DEV(dh_dev, "fw_core_dump_file Write failed\n");
		return;
	}
	while (remaining > 0) {
		size_t chunk = min_t(size_t, remaining, ZXDH_CHUNK_SIZE);
		memcpy_fromio(buf, src, chunk);

#ifdef CGS_V5_693
		ret = kernel_write(file, buf, chunk, pos);
#else
		ret = kernel_write(file, buf, chunk, &pos);
#endif
		if (ret != chunk) {
			HEAL_ERR_DEV(dh_dev, "fw_core_dump_file Write failed\n");
			return;
		}

		src = (uint8_t __iomem *)src + chunk;
		remaining -= chunk;
		cond_resched(); // 允许调度其他任务
	}
}

/**
 * @brief 将SN码数据转换为base16进行存储
 *
 * @param sn_code 输入的SN码数组
 * @param base16_str 输出的base16字符串缓冲区（至少 SN_CODE_LENGTH * 2 + 1 结束符）
 * @param len 缓冲区长度
 */
void sn_code_to_base16_str(uint8_t sn_code[], char *base16_str, size_t len)
{
    uint8_t i;

    if (!sn_code || !base16_str || len < (SN_CODE_LENGTH * 2  + 1)) {
        return;
    }

    for (i = 0; i < SN_CODE_LENGTH; ++i) {
        zte_snprintf_s(&base16_str[i * 2], len - (i * 2), "%02x", sn_code[i]);
    }
}

#define EP4_DUMP_SIZE_MAX (0x10000)
static void fw_log_dump(struct dh_core_dev *dh_dev, uint8_t type)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;
	struct firmware_capability *fwcap = &pf_dev->fwcap;
	const char *dev_name = pci_name(dh_dev->pdev);
	char log_dir[128];
	char filename[256];
	char dir_path[256];
	char base16_str[SN_CODE_LENGTH * 2 + 1] = {0};
	struct file *file;
	uint8_t* buf;
	uint64_t offset;
	uint64_t real_offset;
	uint64_t dump_size;
	uint8_t i = 0;
	int32_t ret;
	size_t chunk = 0;
	size_t remaining = 0;

	if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_COREDUMP) == 1)
	{
		if (FIND_PF_ID(pf_dev->pcie_id) != 0)
		{
			HEAL_ERR_DEV(dh_dev, "Coredump is only supported for PF0. Current PF: %d.\n", FIND_PF_ID(pf_dev->pcie_id));
			return;
		}
	}

	if (pf_dev->pci_ioremap_addr[0] == 0)
	{
		HEAL_ERR_DEV(dh_dev, "pci_ioremap_addr 0:%lld, 2:%lld not ioremap\n", pf_dev->pci_ioremap_addr[0], pf_dev->pci_ioremap_addr[2]);
		return;
	}

	ret = get_zxdh_log_dir(log_dir, sizeof(log_dir));
	if (ret != 0)
	{
		LOG_ERR_DEV(dh_dev, "Failed to get log directory: %d\n", ret);
		return;
	}

	HEAL_DEBUG_DEV(dh_dev, "%s_%s dump start\n", log_name[type], dev_name);

	sn_code_to_base16_str(pf_dev->sn_code, base16_str, sizeof(base16_str));
	zte_snprintf_s(dir_path, sizeof(dir_path), "%s/dhbbx/SN%s/log%d",
			log_dir, base16_str, health->next_load_index);

	zte_snprintf_s(filename, sizeof(filename), "%s/%s_%s.txt", dir_path, log_name[type], dev_name);
	file = filp_open(filename, O_WRONLY | O_CREAT, 0640);
	if (IS_ERR(file)) {
		ret = PTR_ERR(file);
		if (ret == -ENOENT) {
			LOG_INFO_DEV(dh_dev, "File %s does not exist, attempting to create it.\n", filename);
		} else {
			LOG_ERR_DEV(dh_dev, "Error opening file %s: %d\n", filename, ret);
			return;
		}

		// Create directory if it doesn't exist
		ret = create_directory_recursion(dir_path);
		if (ret && ret != -EEXIST) {
			LOG_ERR_DEV(dh_dev, "Failed to create directory %s: %d\n", dir_path, ret);
			return;
		}

		// Reopen file after directory creation
		file = filp_open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0640);
		if (IS_ERR(file)) {
			LOG_ERR_DEV(dh_dev, "Error creating file %s: %ld\n", filename, PTR_ERR(file));
			return;
		}
	}

	file->f_pos = 0;

	if ((type == ZIOS_M7_LOG) || (type == CGEL_M7_LOG)) {
		offset = health->m7_log_offset;
		dump_size = ZXDH_M7_LOG_SIZE;
	} else {
		offset = ZXDH_RISCV_FWLOG_OFFSET;
		dump_size = health->riscv_crdump_size;
	}

	buf = vmalloc(dump_size);
	if (buf == NULL) {
		HEAL_ERR_DEV(dh_dev, "%s vmalloc buf failed\n", pci_name(dh_dev->pdev));
		goto out;
	}

	memset(buf, 0, dump_size);

	if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_COREDUMP) == 1)
	{
		if ((pf_dev->pcie_id & BIT(14)) != 0)
		{
			if (dump_size == ZXDH_M7_LOG_SIZE)
			{
				for (i = 0; (i * EP4_DUMP_SIZE_MAX) < dump_size; ++i)
				{
					chunk = min_t(size_t, EP4_DUMP_SIZE_MAX, dump_size - i * EP4_DUMP_SIZE_MAX);
					real_offset = TO_EP4_ADDR(offset);
					zte_memcpy_s(buf + i * EP4_DUMP_SIZE_MAX, (void __iomem*)(pf_dev->pci_ioremap_addr[0] + real_offset), chunk);
					offset += EP4_DUMP_SIZE_MAX;
				}
			}else
			{
				//1,dump riscv fw log
				for (i = 0; (i * EP4_DUMP_SIZE_MAX) < ZXDH_CGEL_LOG_SIZE; ++i)
				{
					real_offset = TO_EP4_ADDR(offset);
					zte_memcpy_s(buf + i * EP4_DUMP_SIZE_MAX, (void __iomem*)(pf_dev->pci_ioremap_addr[0] + real_offset), EP4_DUMP_SIZE_MAX);
					offset += EP4_DUMP_SIZE_MAX;
				}

#ifdef CGS_V5_693
				ret = kernel_write(file, buf, ZXDH_CGEL_LOG_SIZE, file->f_pos);
#else
				ret = kernel_write(file, buf, ZXDH_CGEL_LOG_SIZE, &file->f_pos);
#endif
				if (ret != ZXDH_CGEL_LOG_SIZE) {
					HEAL_ERR_DEV(dh_dev, "kernel_write riscv bbx log failed, ret:0x%x, pos:0x%llx\n", ret, file->f_pos);
					vfree(buf);
					filp_close(file, NULL);
					return;
				}

				offset = ZXDH_CGEL_LOG_SIZE;
				remaining = dump_size - ZXDH_CGEL_LOG_SIZE;
				if (pf_dev->pci_ioremap_addr[2] == 0)
				{
					HEAL_ERR_DEV(dh_dev, "pci_ioremap_addr 2:%lld not ioremap\n", pf_dev->pci_ioremap_addr[2]);
					vfree(buf);
					filp_close(file, NULL);
					return;
				}
				//2,dump riscv os log
				while (remaining > 0)
				{
					chunk = min_t(size_t, EP4_DUMP_SIZE_MAX, remaining);
					real_offset = TO_EP4_ADDR(offset);
					zte_memcpy_s(buf, (void __iomem*)(pf_dev->pci_ioremap_addr[2] + real_offset), chunk);
#ifdef CGS_V5_693
					ret = kernel_write(file, buf, chunk, file->f_pos);
#else
					ret = kernel_write(file, buf, chunk, &file->f_pos);
#endif
					if (ret != chunk) {
						HEAL_ERR_DEV(dh_dev, "kernel_write failed, ret:0x%x, pos:0x%llx\n", ret, file->f_pos);
						vfree(buf);
						filp_close(file, NULL);
						return;
					}

					offset += chunk;
					remaining = remaining - chunk;

					if (offset % ZXDH_CHUNK_SIZE == 0)
					{
						cond_resched(); // 允许调度其他任务
					}
				}
				vfree(buf);
				filp_close(file, NULL);
				return;
			}
		}else
		{
			if ((type == ZIOS_M7_LOG) || (type == CGEL_M7_LOG))
			{
				zte_memcpy_s(buf, (void __iomem*)(pf_dev->pci_ioremap_addr[0] + offset), dump_size);
			}else
			{
				fw_core_dump_file(pf_dev, file, buf, dump_size);
				vfree(buf);
				filp_close(file, NULL);
				return;
			}
		}
	}
	else if ((pf_dev->pcie_id & BIT(14)) != 0)
	{
		for (i = 0; (i * EP4_DUMP_SIZE_MAX) < dump_size; ++i)
		{
			chunk = min_t(size_t, EP4_DUMP_SIZE_MAX, dump_size - i * EP4_DUMP_SIZE_MAX);
			real_offset = TO_EP4_ADDR(offset);
			zte_memcpy_s(buf + i * EP4_DUMP_SIZE_MAX, (void __iomem*)(pf_dev->pci_ioremap_addr[0] + real_offset), chunk);
			offset += EP4_DUMP_SIZE_MAX;
		}
	}
	else
	{
		memcpy(buf, (void __iomem*)(pf_dev->pci_ioremap_addr[0] + offset), dump_size);
	}

#ifdef CGS_V5_693
	kernel_write(file, buf, dump_size, file->f_pos);
#else
	kernel_write(file, buf, dump_size, &file->f_pos);
#endif

	vfree(buf);
	HEAL_DEBUG_DEV(dh_dev, "%s_%s dump success\n", log_name[type], dev_name);
out:
	filp_close(file, NULL);
}

static void zxdh_m7_bbx_log_dump_work(struct work_struct *work)
{
	struct zxdh_core_health *health = container_of(work, struct zxdh_core_health, m7_bbx_saving_work);
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
	struct firmware_capability *fwcap = &pf_dev->fwcap;

	if (dh_dev->coredev_type == DH_COREDEV_VF)
		return;

	if (IS_STD_BOARD(pf_dev->board_type) ||
		fwcap->os_type == FW_ZIOS_TYPE ||
		fwcap->os_type == FW_CGEL_TO_ZIOS_TYPE)
		fw_log_dump(dh_dev, ZIOS_M7_LOG);
	else
		fw_log_dump(dh_dev, CGEL_M7_LOG);
	health->next_load_index = (health->next_load_index + 1) % 5;
}

static void zxdh_riscv_fw_log_dump_work(struct work_struct *work)
{
	struct zxdh_core_health *health = container_of(work, struct zxdh_core_health, riscv_log_saving_work);
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
	struct firmware_capability *fwcap = &pf_dev->fwcap;

	if (dh_dev->coredev_type == DH_COREDEV_VF)
		return;

	if (IS_STD_BOARD(pf_dev->board_type) ||
		fwcap->os_type == FW_ZIOS_TYPE ||
		fwcap->os_type == FW_CGEL_TO_ZIOS_TYPE)
		fw_log_dump(dh_dev, ZIOS_RISCV_LOG1);
	else
		fw_log_dump(dh_dev, CGEL_RISCV_LOG1);
	health->next_load_index = (health->next_load_index + 1) % 5;
}

static void zxdh_riscv_bbx_log_dump_work(struct work_struct *work)
{
	struct zxdh_core_health *health = container_of(work, struct zxdh_core_health, riscv_bbx_saving_work);
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
	struct firmware_capability *fwcap = &pf_dev->fwcap;

	if (dh_dev->coredev_type == DH_COREDEV_VF)
		return;

	if (IS_STD_BOARD(pf_dev->board_type) ||
		fwcap->os_type == FW_ZIOS_TYPE ||
		fwcap->os_type == FW_CGEL_TO_ZIOS_TYPE)
		fw_log_dump(dh_dev, ZIOS_RISCV_LOG2);
	else
		fw_log_dump(dh_dev, CGEL_RISCV_LOG2);
	health->next_load_index = (health->next_load_index + 1) % 5;
}

static void zxdh_ep_power_state_check_work(struct work_struct *work)
{
	struct delayed_work *delayed_work = to_delayed_work(work);
	struct zxdh_core_health *health = container_of(delayed_work, struct zxdh_core_health, ep_power_state_check_work);
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
	uint32_t i = 0;
	uint16_t state = 0;

	if (dh_dev->coredev_type == DH_COREDEV_VF)
		return;

	for (i = 0; i < 3; i++) {
		pci_read_config_word(dh_dev->pdev, PCI_CB_LEGACY_MODE_BASE, &state);
		if ((state & 0x3) == 0)
			return;
		msleep(100);
	}

	state &= 0xfffc;
	pci_write_config_word(dh_dev->pdev, PCI_CB_LEGACY_MODE_BASE, state);
	health->pstate_change_cnt++;
	LOG_INFO("%s power_state set success\n", pci_name(dh_dev->pdev));
}

static void zxdh_trigger_health_work(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;
	unsigned long flags;
	DPP_PF_INFO_T pf_info = {.slot = pf_dev->slot_id,
							 .vport = pf_dev->vport};

	spin_lock_irqsave(&health->wq_lock, flags);
	if (test_bit(ZXDH_DROP_NEW_HEALTH_WORK, &health->flags)) {
		HEAL_ERR_DEV(dh_dev, "%s new health works are not permitted at this stage\n", pci_name(dh_dev->pdev));
	} else {
		pf_dev->bar_chan_valid = false;
		if (dh_dev->coredev_type == DH_COREDEV_PF)
			dpp_dev_status_set(&pf_info, 0);

		if (health->health_version == 1)
			queue_work(health->wq, &health->fw_fatal_err_work);
		else {
			HEAL_INFO_DEV(dh_dev, "%s selfhealing is not permitted\n", pci_name(dh_dev->pdev));
			if (ioread8(&pf_dev->common->device_status) == 0xb) {
				HEAL_INFO_DEV(dh_dev, "uninstall rdma driver quickly while flr with self-health closed\n");
				zxdh_events_work_enqueue(dh_dev, &pf_dev->rdma_dev_event_work);
			}
		}
	}

	spin_unlock_irqrestore(&health->wq_lock, flags);
}

static void zxdh_riscv_cnt_check(struct core_health *health)
{
	struct zxdh_core_health *dh_health = container_of(health, struct zxdh_core_health, riscv);
	struct zxdh_pf_device *pf_dev = container_of(dh_health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
	struct health_buffer __iomem *hb = health->hb;
	uint32_t count;

	count = ioread32(&hb->health_counter);
	if (count == health->prev)
		++health->miss_counter;
	else
		health->miss_counter = 0;

	health->prev = count;
	if (health->miss_counter == RISCV_MAX_MISSES) {
		HEAL_ERR_DEV(dh_dev, "%s riscv health compromised - reached miss count\n", pci_name(dh_dev->pdev));
		set_bit(RISCV_COUNTER_MISSED, &dh_health->synd);
	} else if (health->miss_counter == RISCV_LOG_DUMP) {
		if (ioread32(&hb->fw_version) == ZXDH_FOUR_BYTE_FF) {
			HEAL_ERR_DEV(dh_dev, "%s Bar space Abnomaly, no need to store logs\n", pci_name(dh_dev->pdev));
			return;
		}
		queue_work(dh_health->wq, &dh_health->riscv_log_saving_work);
	} else if (health->miss_counter == RISCV_BBX_DUMP) {
		queue_work(dh_health->wq, &dh_health->riscv_bbx_saving_work);
		queue_delayed_work(dh_health->wq, &dh_health->ep_power_state_check_work, msecs_to_jiffies(10000));
	}
}

static void zxdh_m7_cnt_check(struct core_health *health)
{
	struct health_buffer __iomem *hb = health->hb;
	struct zxdh_core_health *dh_health = container_of(health, struct zxdh_core_health, m7);
	struct zxdh_pf_device *pf_dev = container_of(dh_health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
	uint32_t count;

	count = ioread32(&hb->health_counter);
	if (count == 0)
		return;
	else if (count == health->prev)
		++health->miss_counter;
	else
		health->miss_counter = 0;

	health->prev = count;
	if (health->miss_counter == M7_MAX_MISSES) {
		HEAL_ERR_DEV(dh_dev, "%s m7 health compromised - reached miss count\n", pci_name(dh_dev->pdev));
		set_bit(M7_COUNTER_MISSED, &dh_health->synd);
	} else if (health->miss_counter == M7_LOGDUMP) {
		queue_work(dh_health->wq, &dh_health->m7_bbx_saving_work);
	}
}

#define MAX_DETECT_CNT 3
static bool sensor_bar_error(struct zxdh_core_health *health)
{
	struct health_buffer __iomem *hb = health->riscv.hb;
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);

	if (ioread32(&hb->fw_version) == ZXDH_FOUR_BYTE_FF) {
		health->fatal_detect_cnt++;
		HEAL_INFO_DEV(dh_dev, "%s bar_err_detect_cnt: %d\n", pci_name(dh_dev->pdev), health->fatal_detect_cnt);
	} else
		health->fatal_detect_cnt = 0;

	if (health->fatal_detect_cnt == MAX_DETECT_CNT) {
		health->reset_done = true;
		return true;
	}

	return false;
}

static inline bool sensor_fw_synd_rfr(struct zxdh_core_health *health)
{
	struct health_buffer __iomem *hb = health->riscv.hb;
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);

	if (ioread8(&hb->rfr) != 1)
		return false;

	if ((dh_dev->coredev_type == DH_COREDEV_PF) && (health->health_version == 1))
		queue_work(health->wq, &health->dh_reset_work);

	return true;
}

static inline bool sensor_fw_exception(const struct core_health *health)
{
	struct health_buffer __iomem *hb = health->hb;

	return (ioread8(&hb->fw_exception) == 1);
}

static bool sensor_dh_fw_exception(struct zxdh_core_health *health)
{
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
	int32_t i = 0;
	struct {
		struct core_health *core;
		struct work_struct *work;
		const char *name;
	} cores[] = {
		{&health->riscv, &health->riscv_bbx_saving_work, "riscv"},
		{&health->m7, &health->m7_bbx_saving_work, "m7"}
	};

	for (i = 0; i < ARRAY_SIZE(cores); i++) {
		if (sensor_fw_exception(cores[i].core)) {
			pf_dev->bar_chan_valid = false;
			HEAL_ERR_DEV(dh_dev, "%s %s fw_exception\n", pci_name(dh_dev->pdev), cores[i].name);
			if (!queue_work(health->wq, cores[i].work)) {
				HEAL_ERR_DEV(dh_dev, "%s Failed to queue work for %s\n", pci_name(dh_dev->pdev), cores[i].name);
				continue;
			}
			return 1;
		}
	}
	return 0;
}

static inline bool sensor_dh_reset_ok(struct core_health *health)
{
	struct health_buffer __iomem *hb = health->hb;

	return (ioread8(&hb->riscv_power_on) == 1);
}

static bool sensor_flr_reset(struct zxdh_core_health *health)
{
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);

	if (ioread8(&pf_dev->common->device_status) != 0xb)
		return false;

	health->reset_done = true;
	return true;
}

static bool sensor_cnt_missed(struct zxdh_core_health *health)
{
	if (test_bit(RISCV_COUNTER_MISSED, &health->synd))
		return true;

	if (test_bit(M7_COUNTER_MISSED, &health->synd))
		return true;

	return false;
}

static void update_synd_statics(struct zxdh_core_health *health, uint64_t synd)
{
	int32_t i = 0;

	for (i = 0; i < 64; i++) {
		if ((synd >> i) & 1) {
			health->synd_statics[i]++;
		}
	}
}

static void zxdh_synd_detect(struct zxdh_core_health *health)
{
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
	struct core_health *riscv = &health->riscv;
	struct core_health *m7 = &health->m7;
	struct health_buffer __iomem *hb = NULL;
	uint32_t prev_synd;
	uint64_t total_synd;
	uint64_t prev_total_synd;

	prev_total_synd = health->synd;

	hb = riscv->hb;
	prev_synd = riscv->synd;
	riscv->synd = ioread32(&hb->synd);
	if (riscv->synd && riscv->synd != ZXDH_FOUR_BYTE_FF && riscv->synd != prev_synd) {
		HEAL_ERR_DEV(dh_dev, "%s riscv->synd 0x%x\n", pci_name(dh_dev->pdev), riscv->synd);
		health->synd |= riscv->synd;
	}

	if (health->health_version != 1)
		goto out;

	hb = m7->hb;
	prev_synd = m7->synd;
	m7->synd = ioread32(&hb->synd);
	if (m7->synd && m7->synd != ZXDH_FOUR_BYTE_FF && m7->synd != prev_synd) {
		HEAL_ERR_DEV(dh_dev, "%s m7->synd 0x%x\n", pci_name(dh_dev->pdev), m7->synd);
		total_synd = m7->synd;
		health->synd |= (total_synd << 32);
	}

out:
	if (health->synd && health->synd != prev_total_synd)
		update_synd_statics(health, health->synd ^ prev_total_synd);
}

static void zxdh_health_sync(struct zxdh_core_health *health, uint8_t synd)
{
	set_bit(synd, &health->synd);
	update_synd_statics(health, (uint64_t)1 << (synd));
}

struct sensor_check {
	bool (*func)(struct zxdh_core_health *);
	const char *error_msg;
	uint32_t sync_code;
} const checks[] = {
	{sensor_bar_error, "bar error", BAR_ERROR},
	{sensor_flr_reset, "sensor_flr_reset", FLR_RESET},
	{sensor_fw_synd_rfr, "fw need rfr", INVALID_SYND},
	{sensor_dh_fw_exception, "sensor_dh_fw_exception", INVALID_SYND},
	{sensor_cnt_missed, "sensor_cnt_missed", INVALID_SYND},
};

uint8_t zxdh_health_check_fatal_sensors(struct zxdh_core_health *health)
{
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
	uint8_t i = 0;
	uint8_t check_num = ARRAY_SIZE(checks);

	if (health->fatal)
		check_num = 2;

	for (i = 0; i < check_num; i++) {
		if (checks[i].func(health)) {
			HEAL_ERR_DEV(dh_dev, "%s %s\n", pci_name(dh_dev->pdev), checks[i].error_msg);
			if (checks[i].sync_code == INVALID_SYND) {
				zxdh_synd_detect(health);
			} else {
				zxdh_health_sync(health, checks[i].sync_code);
			}
			return health->fatal++;
		}
	}

	return health->fatal;
}

#define ZXDH_HEALTH_MAX_WAIT_MSECS 600000
#define ZXDH_WAIT_CONDITION(condition, stop_valid)                              \
do {                                                                            \
	unsigned long end = jiffies + msecs_to_jiffies(ZXDH_HEALTH_MAX_WAIT_MSECS); \
	while (!(condition))                                                        \
	{                                                                           \
		if ((stop_valid) == ZXDH_REMOVE)                                        \
			return -ETIMEDOUT;                                                  \
		if (time_after(jiffies, end))                                           \
			return -ETIMEDOUT;                                                  \
		msleep(1000);                                                           \
	}                                                                           \
} while (0)

int zxdh_health_wait_dh_ok(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;

	ZXDH_WAIT_CONDITION(sensor_dh_reset_ok(&health->riscv), dh_dev->driver_process);
	HEAL_INFO_DEV(dh_dev, "%s dh is ok\n", pci_name(dh_dev->pdev));
	return 0;
}

int wait_vital(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;
	struct health_buffer __iomem *hb = health->riscv.hb;
	const int niter = 600;
	u32 last_count = 0;
	u32 count;
	int i;

	for (i = 0; i < niter; i++) {
		count = ioread32(&hb->health_counter);
		if (count && count != 0xffffffff) {
			if (last_count && last_count != count) {
				HEAL_INFO_DEV(dh_dev, "%s wait vital counter value 0x%x after %d iterations\n",
						pci_name(dh_dev->pdev), count, i);
				return 0;
			}
			last_count = count;
		}
		if (dh_dev->driver_process == ZXDH_REMOVE)
			return -ETIMEDOUT;

		msleep(1000);
	}

	return -ETIMEDOUT;
}

static inline bool dh_reload_confirm(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;
	DPP_PF_INFO_T pf_info = {.slot = pf_dev->slot_id,
								.vport = pf_dev->vport};
	uint8_t state = 0;

	if (!sensor_flr_reset(health))
	{
		pf_dev->bar_chan_valid = true;
		dpp_dev_status_set(&pf_info, 1);
		dh_dev->device_state = ZXDH_DEVICE_STATE_UP;
		state = ZXDH_DEVICE_STATE_RELOAD;
		zxdh_pf_call_aux_events_with_data(dh_dev, DH_EVENT_TYPE_AUX_STATE, &state);
		return false;
	}

	return true;
}

int dh_pf_wait_riscv_ready(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;
	int i = 0;

	health->riscv.hb = (struct health_buffer __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_RISCV_HB_OFFSET);
	health->m7.hb = (struct health_buffer __iomem *)(pf_dev->pci_ioremap_addr[0] + ZXDH_M7_HB_OFFSET);

	dh_health_version_get(health);

	if ((health->health_version != 1) &&
		(pf_dev->fw_compat.patch < DH_HPIRQ_PATCH)) {
		HEAL_INFO_DEV(dh_dev, "%s riscv_power_on not valid\n", pci_name(dh_dev->pdev));
		return 0;
	}

	for (i = 0; i < 40; ++i) {
		if (sensor_dh_reset_ok(&health->riscv)) {
			HEAL_INFO_DEV(dh_dev, "%s wait %ds\n", pci_name(dh_dev->pdev), i);
			return 0;
		}

		if (dh_dev->driver_process == ZXDH_REMOVE)
			return -ETIMEDOUT;

		msleep(1000);
	}

	return -1;
}

int zxdh_vf_wait_pf_ok(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;
	struct health_buffer __iomem *hb = health->riscv.hb;
	uint8_t ep_id = (pf_dev->pcie_id >> 12) & 0x7;
	uint8_t pf_id = (pf_dev->pcie_id >> 8) & 0x7;
	uint8_t pf_ok = 1 << pf_id;
	ZXDH_WAIT_CONDITION((ioread8(&hb->pf_status[ep_id]) & pf_ok) != 0, dh_dev->driver_process);
	return 0;
}

static int zxdh_pf_health_msg_send(struct dh_core_dev *dh_dev, union zxdh_msg *msg)
{
	struct zxdh_bar_extra_para para = {0};
	para.is_sync = true;
	para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

	return zxdh_pf_msg_send_cmd(dh_dev, MODULE_HEALTH, msg, msg, &para);
}

int zxdh_pf_status_ok(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	union zxdh_msg *msg = NULL;
	int32_t err = 0;

	if (dh_dev->coredev_type == DH_COREDEV_VF)
		return 0;

	msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
	if (msg == NULL) {
		HEAL_ERR_DEV(dh_dev, "%s kzalloc(%lu, GFP_KERNEL) failed\n", pci_name(dh_dev->pdev), sizeof(union zxdh_msg));
		return -1;
	}

	msg->payload.health_hdr.opcode = 1;
	msg->payload.pf_status_msg.pcie_id = pf_dev->pcie_id;
	msg->payload.health_hdr.sum_check = sum_func(&msg->payload.pf_status_msg, 2);

	err = zxdh_pf_health_msg_send(dh_dev, msg);
	kfree(msg);
	return err;
}

#define PCIE_CONFIG_STORE  (0x1c)
int zxdh_pf_pcie_config_store(struct dh_core_dev *dh_dev)
{
	union zxdh_msg *msg = NULL;
	int32_t err = 0;

	if (dh_dev->coredev_type == DH_COREDEV_VF)
		return 0;

	msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
	if (msg == NULL) {
		HEAL_ERR_DEV(dh_dev, "%s kzalloc(%lu, GFP_KERNEL) failed\n", pci_name(dh_dev->pdev), sizeof(union zxdh_msg));
		return -1;
	}

	msg->payload.health_hdr.opcode = 0;
	msg->payload.health_config_msg.act = PCIE_CONFIG_STORE;
	msg->payload.health_hdr.sum_check = PCIE_CONFIG_STORE;

	err = zxdh_pf_health_msg_send(dh_dev, msg);
	kfree(msg);
	return err;
}

#define DH_RESET_REQUEST  (0x1a)
static int32_t zxdh_pf_dh_reset_request(struct dh_core_dev *dh_dev)
{
	union zxdh_msg *msg = NULL;
	int32_t err = 0;

	if (dh_dev->coredev_type == DH_COREDEV_VF)
		return 0;

	msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
	if (msg == NULL) {
		HEAL_ERR_DEV(dh_dev, "%s kzalloc(%lu, GFP_KERNEL) failed\n", pci_name(dh_dev->pdev), sizeof(union zxdh_msg));
		return -1;
	}

	msg->payload.health_hdr.opcode = 0;
	msg->payload.health_config_msg.act = DH_RESET_REQUEST;
	msg->payload.health_hdr.sum_check = DH_RESET_REQUEST;

	err = zxdh_pf_health_msg_send(dh_dev, msg);
	HEAL_INFO_DEV(dh_dev, "%s dh reset request, err = %d\n", pci_name(dh_dev->pdev), err);
	kfree(msg);
	return err;
}


static void poll_health(struct timer_list *t)
{
	struct zxdh_core_health *health = from_timer(health, t, timer);
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
	uint8_t fatal = 0;

	if (pf_dev->aux_comp_flag == 0)
		goto out;

	dh_health_version_update(health);

	fatal = zxdh_health_check_fatal_sensors(health);
	if (fatal != health->fatal) {
		dh_dev->device_state = ZXDH_DEVICE_STATE_INTERNAL_ERROR;
		zxdh_pf_call_aux_events_with_data(dh_dev, DH_EVENT_TYPE_AUX_STATE, &dh_dev->device_state);
		if (health->reset_done) {
			HEAL_ERR_DEV(dh_dev, "%s Fatal error detected: %d\n", pci_name(dh_dev->pdev), health->fatal);
			/* ovs需要根据日志中的“Fatal error detected”来获取自愈次数，
				请不要修改此内容和该内容出现的次数 */
			return zxdh_trigger_health_work(dh_dev);
		}
	}

	zxdh_riscv_cnt_check(&health->riscv);
	if (health->health_version == 1)
		zxdh_m7_cnt_check(&health->m7);
	zxdh_synd_detect(health);

out:
	mod_timer(&health->timer, jiffies + ZXDH_HEALTH_POLL_INTERVAL);
}

static void zxdh_start_health_poll(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;

	clear_bit(ZXDH_DROP_NEW_HEALTH_WORK, &health->flags);
	health->synd = 0;
	health->fatal = 0;
	health->reset_done = false;
	health->fatal_detect_cnt = 0;
	health->next_load_index = 0;
	mod_timer(&health->timer, jiffies + ZXDH_HEALTH_POLL_INTERVAL);
}

static int zxdh_health_try_recover(struct dh_core_dev *dh_dev)
{
	uint32_t value = 0;

	HEAL_INFO_DEV(dh_dev, "%s handling bad device here\n", pci_name(dh_dev->pdev));
	if (dh_core_is_vf(dh_dev))
		goto load;

	do {
		pci_read_config_dword(dh_dev->pdev, 0, &value);
		if (dh_dev->driver_process == ZXDH_REMOVE)
			return -ETIMEDOUT;
		if (value != 0xffffffff)
			break;
		msleep(1000);
	} while (1);

	if (!zxdh_pf_pcie_config_reload_check(dh_dev))
		goto load;

	if (pci_enable_device(dh_dev->pdev) != 0) {
		LOG_ERR_DEV(dh_dev, "pci_enable_device failed\n");
		return -EIO;
	}
	pci_set_master(dh_dev->pdev);
	pci_restore_state(dh_dev->pdev);
	pci_save_state(dh_dev->pdev);

load:
	if (wait_vital(dh_dev)) {
		HEAL_ERR_DEV(dh_dev, "%s wait_vital time out\n", pci_name(dh_dev->pdev));
		return -EIO;
	}
	if (zxdh_health_wait_dh_ok(dh_dev)) {
		HEAL_ERR_DEV(dh_dev, "%s zxdh_health_wait_dh_ok time out\n", pci_name(dh_dev->pdev));
		return -EIO;
	}
	if (!dh_reload_confirm(dh_dev)) {
		HEAL_INFO_DEV(dh_dev, "%s no need to reload\n", pci_name(dh_dev->pdev));
		goto out;
	}

	if (zxdh_pf_pcie_config_reload_check(dh_dev)) {
		HEAL_ERR_DEV(dh_dev, "%s zxdh_pf_pcie_config reload failed\n", pci_name(dh_dev->pdev));
		return -EIO;
	}

	zxdh_unload_one(dh_dev);
	HEAL_INFO_DEV(dh_dev, "%s zxdh_unload_one finish\n", pci_name(dh_dev->pdev));
	if (zxdh_load_one(dh_dev)) {
		HEAL_ERR_DEV(dh_dev, "%s zxdh_load_one failed\n", pci_name(dh_dev->pdev));
		return -EIO;
	}

out:
	zxdh_start_health_poll(dh_dev);
	return 0;
}

static void zxdh_dh_reset_work(struct work_struct *work)
{
	struct zxdh_core_health *health = container_of(work, struct zxdh_core_health, dh_reset_work);
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);

	zxdh_pf_dh_reset_request(dh_dev);
}

static void zxdh_fw_fatal_err_work(struct work_struct *work)
{
	struct zxdh_core_health *health = container_of(work, struct zxdh_core_health, fw_fatal_err_work);
	struct zxdh_pf_device *pf_dev = container_of(health, struct zxdh_pf_device, health);
	struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);

	HEAL_INFO_DEV(dh_dev, "%s zxdh_fw_fatal_err_work start\n", pci_name(dh_dev->pdev));
	zxdh_health_try_recover(dh_dev);
}

void zxdh_drain_health_wq(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;
	unsigned long flags;

	if (!health->health_supported)
		return;

	spin_lock_irqsave(&health->wq_lock, flags);
	set_bit(ZXDH_DROP_NEW_HEALTH_WORK, &health->flags);
	spin_unlock_irqrestore(&health->wq_lock, flags);
	cancel_work_sync(&health->fw_fatal_err_work);
	cancel_work_sync(&health->dh_reset_work);
	cancel_work_sync(&health->m7_bbx_saving_work);
	cancel_work_sync(&health->riscv_bbx_saving_work);
	cancel_work_sync(&health->riscv_log_saving_work);
	cancel_delayed_work_sync(&health->ep_power_state_check_work);
	cancel_work_sync(&pf_dev->rdma_dev_proc_work);
	cancel_work_sync(&pf_dev->rdma_dev_event_work);
}

static void zxdh_stop_health_poll(struct dh_core_dev *dh_dev, bool disable_health)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;
	unsigned long flags;

	if (disable_health) {
		spin_lock_irqsave(&health->wq_lock, flags);
		set_bit(ZXDH_DROP_NEW_HEALTH_WORK, &health->flags);
		spin_unlock_irqrestore(&health->wq_lock, flags);
	}

	del_timer_sync(&health->timer);
}

void zxdh_health_cleanup(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;

	if (!health->health_supported)
		return;

	zxdh_stop_health_poll(dh_dev, true);
	destroy_workqueue(health->wq);
	zxdh_health_attr_remove(dh_dev);
}

int zxdh_crdump_size_get(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct firmware_capability *fwcap = &pf_dev->fwcap;
	uint8_t board_type = pf_dev->board_type;
	struct zxdh_core_health *health = &pf_dev->health;

	if (IS_STD_BOARD(board_type) || IS_STORAGE_BOARD(board_type) || \
		IS_GPU_BOARD(board_type)) {
		health->m7_log_offset = ZXDH_M7_ZIOS_LOG_OFFSET;
		health->riscv_crdump_size = ZXDH_ZIOS_LOG_SIZE;
	} else if (IS_INIC_BOARD(board_type)) {
		health->m7_log_offset = ZXDH_M7_CGEL_LOG_OFFSET;
		health->riscv_crdump_size = ZXDH_CGEL_LOG_SIZE;
	} else {
		HEAL_INFO_DEV(dh_dev, "%s board_type %d not support\n", pci_name(dh_dev->pdev), board_type);
		return -1;
	}

	if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_COREDUMP) == 1) {
		if (FIND_PF_ID(pf_dev->pcie_id) == 0)
		{
			health->riscv_crdump_size = ZXDH_CGEL_ZIOS_SIZE;
			if ((dh_dev->coredev_type == DH_COREDEV_PF) &&
				(ZXDH_CGEL_ZIOS_SIZE > pci_resource_len(dh_dev->pdev, 2))) {
				HEAL_ERR_DEV(dh_dev, "%s pci_resource_len: %llx\n",pci_name(dh_dev->pdev), pci_resource_len(dh_dev->pdev, 2));
				return -1;
			}
		}
	}else {
		if ((dh_dev->coredev_type == DH_COREDEV_PF) &&
			(ZXDH_RISCV_FWLOG_OFFSET + health->riscv_crdump_size > pci_resource_len(dh_dev->pdev, 0))) {
			HEAL_ERR_DEV(dh_dev, "%s pci_resource_len: %llx\n",pci_name(dh_dev->pdev), pci_resource_len(dh_dev->pdev, 0));
			return -1;
		}
	}

	return 0;
}

int zxdh_health_init(struct dh_core_dev *dh_dev)
{
	struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);
	struct zxdh_core_health *health = &pf_dev->health;
	char *name;
	int err = 0;

	if (health->health_version > 1)
		goto out;

	if (zxdh_crdump_size_get(dh_dev))
		goto out;

	name = kmalloc(64, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	strcpy(name, "zxdh_health");
	strcat(name, dev_name(dh_dev->device));
	health->wq = create_singlethread_workqueue(name);
	kfree(name);

	if (!health->wq)
		return -ENOMEM;

	spin_lock_init(&health->wq_lock);
	INIT_WORK(&health->fw_fatal_err_work, zxdh_fw_fatal_err_work);
	INIT_WORK(&health->dh_reset_work, zxdh_dh_reset_work);
	INIT_WORK(&health->m7_bbx_saving_work, zxdh_m7_bbx_log_dump_work);
	INIT_WORK(&health->riscv_log_saving_work, zxdh_riscv_fw_log_dump_work);
	INIT_WORK(&health->riscv_bbx_saving_work, zxdh_riscv_bbx_log_dump_work);
	INIT_DELAYED_WORK(&health->ep_power_state_check_work, zxdh_ep_power_state_check_work);

	err = zxdh_pf_rp_config_init(dh_dev);
	if (err != 0)
		HEAL_ERR_DEV(dh_dev, "%s zxdh_pf_rp_config_init failed: %d\n", pci_name(dh_dev->pdev), err);

	err = zxdh_pf_status_ok(dh_dev);
	if (err != 0)
		HEAL_ERR_DEV(dh_dev, "%s zxdh_pf_status_ok failed: %d\n", pci_name(dh_dev->pdev), err);

	if (zxdh_health_attr_create(dh_dev) != 0)
		goto destroy_wq;

	timer_setup(&health->timer, poll_health, 0);
	health->health_supported = true;
	zxdh_start_health_poll(dh_dev);
	return 0;

destroy_wq:
	destroy_workqueue(health->wq);
	return -ENOEXEC;
out:
	health->health_supported = false;
	HEAL_INFO_DEV(dh_dev, "%s health buffer not supported\n", pci_name(dh_dev->pdev));
	return 0;
}
