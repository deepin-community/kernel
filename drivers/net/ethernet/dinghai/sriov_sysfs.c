#include <linux/dinghai/zxdh_auxiliary_bus.h>
#include <linux/dinghai/driver.h>
#include <net/udp_tunnel.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/netdevice.h>
#include "en_aux.h"
#include <linux/etherdevice.h>
#include "en_np/table/include/dpp_tbl_api.h"
#include "en_np/table/include/dpp_tbl_comm.h"
#include "en_aux/en_aux_cmd.h"
#include "msg_common.h"
#include "en_pf.h"
#include "en_pf/en_pf_eq.h"
#include <linux/dinghai/kcompat.h>
#ifdef TIME_STAMP_1588
#include "en_aux/en_1588_pkt_proc.h"
#endif
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>


#define ZXDH_DEFAULT_VF_GROUP_ID 0
#define ZXDH_MAX_VF_GROUP_OBJ_ID 255
#define ZXDH_MAX_VF_GROUP_OBJ_NUM (ZXDH_MAX_VF_GROUP_OBJ_ID + 1)
#define ZXDH_MAX_VF_OBJ_ID 255
#define ZXDH_MAX_VF_OBJ_NUM (ZXDH_MAX_VF_OBJ_ID + 1)

#ifdef NEED_SYSFS_EMIT
int sysfs_emit(char *buf, const char *fmt, ...)
{
    int ret;
    va_list args;

    if (WARN(!buf || offset_in_page(buf),
            "invalid sysfs_emit: buf:%p\n", buf))
        return 0;

    va_start(args, fmt);
    ret = vscnprintf(buf, PAGE_SIZE, fmt, args);
    va_end(args);
    return ret;
}
#endif

/************************************************
 * sriov attr：
 *   burst: WO，配置当前PF的流量突发尺寸
 *   profile: RO，打印当前PF对三级car的profile占用情况
************************************************/


/************************************************
 * group attr：
 *   config: RO，打印当前vf端口组所有配置
 *   max_rx_rate: WO，配置当前vf端口组最大接收速率
 *   max_tx_rate: WO，配置当前vf端口组最大发送速率
************************************************/


/************************************************
 * vf attr：
 * config: RO，打印当前vf所有配置
 * group: WO，指定当前vf端口所属group，默认group-0
 * meter
 *   rx
 *      bps
 *          max_rate: WO，配置当前vf端口接收方向字节限速
 *          min_rate: WO，配置当前vf端口接收方向字节最小保障带宽
 *          pkt_dropped: RO, 查询丢包数量/字节数
 *      pps
 *          rate: WO，配置当前vf端口接收方向包限速
 *          pkt_dropped: RO, 查询丢包数量/字节数
 *   tx
 *      bps
 *          max_rate: WO，配置当前vf端口发送方向字节限速
 *          max_rate: WO，配置当前vf端口发送方向字节最小保障带宽
 *          pkt_dropped: RO, 查询丢包数量/字节数

 *      pps
 *          pps_rate: WO，配置当前vf端口发送方向包限速
 *          pkt_dropped: RO, 查询丢包数量/字节数
************************************************/


struct zxdh_group_attribute {
    struct attribute attr;
    ssize_t (*show)(struct zxdh_group_obj *group,
                    struct zxdh_group_attribute *attr, char *buf);
    ssize_t (*store)(struct zxdh_group_obj *group,
                    struct zxdh_group_attribute *attr, const char *buf, size_t count);
};

#define to_zxdh_group_attr(x) container_of(x, struct zxdh_group_attribute, attr)
#define to_zxdh_group_obj(x) container_of(x, struct zxdh_group_obj, kobj)

struct zxdh_vf_attribute {
    struct attribute attr;
    ssize_t (*show)(struct zxdh_vf_obj *vf,
                    struct zxdh_vf_attribute *attr, char *buf);
    ssize_t (*store)(struct zxdh_vf_obj *vf,
                    struct zxdh_vf_attribute *attr, const char *buf, size_t count);
};

#define to_zxdh_vf_attr(x) container_of(x, struct zxdh_vf_attribute, attr)
#define to_zxdh_vf_obj(x) container_of(x, struct zxdh_vf_obj, kobj)

struct zxdh_vf_meter_attribute {
    struct attribute attr;
    ssize_t (*show)(struct zxdh_vf_meter_obj *xps,
                    struct zxdh_vf_meter_attribute *attr, char *buf);
    ssize_t (*store)(struct zxdh_vf_meter_obj *xps,
                    struct zxdh_vf_meter_attribute *attr, const char *buf, size_t count);
};

#define to_zxdh_vf_meter_attr(x) container_of(x, struct zxdh_vf_meter_attribute, attr)
#define to_zxdh_vf_meter_obj(x) container_of(x, struct zxdh_vf_meter_obj, kobj)

/*
 * The default show function that must be passed to sysfs.  This will be
 * called by sysfs for whenever a show function is called by the user on a
 * sysfs file associated with the kobjects we have registered.  We need to
 * transpose back from a "default" kobject to our custom struct foo_obj and
 * then call the show function for that specific object.
 */
static ssize_t zxdh_group_attr_show(struct kobject *kobj,
                 struct attribute *attr,
                 char *buf)
{
    struct zxdh_group_attribute *attribute;
    struct zxdh_group_obj *group;

    attribute = to_zxdh_group_attr(attr);
    group = to_zxdh_group_obj(kobj);

    if (!attribute->show)
        return -EIO;

    return attribute->show(group, attribute, buf);
}

/*
 * Just like the default show function above, but this one is for when the
 * sysfs "store" is requested (when a value is written to a file.)
 */
static ssize_t zxdh_group_attr_store(struct kobject *kobj,
                  struct attribute *attr,
                  const char *buf, size_t len)
{
    struct zxdh_group_attribute *attribute;
    struct zxdh_group_obj *group;

    attribute = to_zxdh_group_attr(attr);
    group = to_zxdh_group_obj(kobj);

    if (!attribute->store)
        return -EIO;

    return attribute->store(group, attribute, buf, len);
}

static ssize_t zxdh_vf_attr_show(struct kobject *kobj,
                 struct attribute *attr,
                 char *buf)
{
    struct zxdh_vf_attribute *attribute;
    struct zxdh_vf_obj *vf;

    attribute = to_zxdh_vf_attr(attr);
    vf = to_zxdh_vf_obj(kobj);

    if (!attribute->show)
        return -EIO;

    return attribute->show(vf, attribute, buf);
}

static ssize_t zxdh_vf_attr_store(struct kobject *kobj,
                  struct attribute *attr,
                  const char *buf, size_t len)
{
    struct zxdh_vf_attribute *attribute;
    struct zxdh_vf_obj *vf;

    attribute = to_zxdh_vf_attr(attr);
    vf = to_zxdh_vf_obj(kobj);

    if (!attribute->store)
        return -EIO;

    return attribute->store(vf, attribute, buf, len);
}

static ssize_t zxdh_vf_meter_attr_show(struct kobject *kobj,
                 struct attribute *attr,
                 char *buf)
{
    struct zxdh_vf_meter_attribute *attribute;
    struct zxdh_vf_meter_obj *xps;

    attribute = to_zxdh_vf_meter_attr(attr);
    xps = to_zxdh_vf_meter_obj(kobj);

    if (!attribute->show)
        return -EIO;

    return attribute->show(xps, attribute, buf);
}

static ssize_t zxdh_vf_meter_attr_store(struct kobject *kobj,
                  struct attribute *attr,
                  const char *buf, size_t len)
{
    struct zxdh_vf_meter_attribute *attribute;
    struct zxdh_vf_meter_obj *xps;

    attribute = to_zxdh_vf_meter_attr(attr);
    xps = to_zxdh_vf_meter_obj(kobj);

    if (!attribute->store)
        return -EIO;

    return attribute->store(xps, attribute, buf, len);
}

static ssize_t zxdh_vf_stats_update(DPP_PF_INFO_T pf_info, int vf_idx, struct zxdh_pf_device *pf_dev, struct zxdh_vf_file_stats *vf_file_stats)
{
    struct zxdh_vf_item *vf_item = NULL;
    struct zxdh_en_vport_np_stats *np_stats = NULL;
    union zxdh_msg *msg = NULL;
    uint32_t vf_id = EPID(pf_info.vport) * 256 + VFUNC_NUM(pf_info.vport);
    uint16_t vf_pcie_id = FIND_VF_PCIE_ID(pf_dev->pcie_id, vf_idx);
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = 0;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(dh_dev, "kzalloc(%lu, GFP_KERNEL) failed !\n", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    //VQM统计
    msg->payload.hdr_to_agt.op_code = AGENT_VQM_DEVICE_STATS_GET;
    msg->payload.hdr_to_agt.vf_id = vf_id;
    msg->payload.hdr_to_agt.pcie_id = vf_pcie_id;
    err = zxdh_pf_msg_send_cmd(dh_dev, MODULE_VQM, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "vfid %d zxdh_vqm_stats_get failed, err: %d\n", vf_id, err);
        kfree(msg);
        return -1;
    }

    vf_item = zxdh_pf_get_vf_item(dh_dev, vf_idx);
    np_stats = kzalloc(sizeof(struct zxdh_en_vport_np_stats), GFP_KERNEL);
    if (np_stats == NULL)
    {
        LOG_ERR_DEV(dh_dev, "kzalloc(%lu, GFP_KERNEL) failed !\n", sizeof(struct zxdh_en_vport_np_stats));
        kfree(msg);
        return -ENOMEM;
    }
    LOG_DEBUG_DEV(dh_dev, "zxdh_vf_stats_update is called, vport: 0x%x, vf_id %d, pf_info.slot %u\n",
                pf_info.vport, vf_id, pf_info.slot);

    dpp_stat_port_mc_packet_rx_cnt_get(&pf_info, vf_id, NP_GET_PKT_CNT, &(np_stats->rx_vport_multicast_bytes), &(np_stats->rx_vport_multicast_packets));
    dpp_stat_port_mc_packet_tx_cnt_get(&pf_info, vf_id, NP_GET_PKT_CNT, &(np_stats->tx_vport_multicast_bytes), &(np_stats->tx_vport_multicast_packets));
    dpp_stat_port_bc_packet_rx_cnt_get(&pf_info, vf_id, NP_GET_PKT_CNT, &(np_stats->rx_vport_broadcast_bytes), &(np_stats->rx_vport_broadcast_packets));
    dpp_stat_port_bc_packet_tx_cnt_get(&pf_info, vf_id, NP_GET_PKT_CNT, &(np_stats->tx_vport_broadcast_bytes), &(np_stats->tx_vport_broadcast_packets));
    dpp_stat_MTU_packet_msg_rx_cnt_get(&pf_info, vf_id, NP_GET_PKT_CNT, &(np_stats->rx_vport_mtu_drop_bytes), &(np_stats->rx_vport_mtu_drop_packets));
    dpp_stat_MTU_packet_msg_tx_cnt_get(&pf_info, vf_id, NP_GET_PKT_CNT, &(np_stats->tx_vport_mtu_drop_bytes), &(np_stats->tx_vport_mtu_drop_packets));
    dpp_stat_plcr_packet_drop_rx_cnt_get(&pf_info, vf_id, NP_GET_PKT_CNT, &(np_stats->rx_vport_plcr_drop_bytes), &(np_stats->rx_vport_plcr_drop_packets));
    dpp_stat_plcr_packet_drop_tx_cnt_get(&pf_info, vf_id, NP_GET_PKT_CNT, &(np_stats->tx_vport_plcr_drop_bytes), &(np_stats->tx_vport_plcr_drop_packets));

    dpp_stat_port_uc_packet_rx_cnt_get(&pf_info, vf_id, NP_GET_PKT_CNT, &(np_stats->rx_vport_unicast_bytes), &(np_stats->rx_vport_unicast_packets));
    dpp_stat_port_uc_packet_tx_cnt_get(&pf_info, vf_id, NP_GET_PKT_CNT, &(np_stats->tx_vport_unicast_bytes), &(np_stats->tx_vport_unicast_packets));

    vf_file_stats->tx_packets += (np_stats->tx_vport_multicast_packets + np_stats->tx_vport_broadcast_packets +
                                  np_stats->tx_vport_unicast_packets);
    vf_file_stats->tx_bytes += (np_stats->tx_vport_multicast_bytes + np_stats->tx_vport_broadcast_bytes +
                                np_stats->tx_vport_unicast_bytes);
    vf_file_stats->rx_packets += (np_stats->rx_vport_multicast_packets + np_stats->rx_vport_broadcast_packets +
                                  np_stats->rx_vport_unicast_packets);
    vf_file_stats->rx_bytes += (np_stats->rx_vport_multicast_bytes + np_stats->rx_vport_broadcast_bytes +
                                np_stats->rx_vport_unicast_bytes);
    vf_file_stats->rx_broadcast += np_stats->rx_vport_broadcast_packets;
    vf_file_stats->rx_multicast += np_stats->rx_vport_multicast_packets;
    vf_file_stats->tx_broadcast += np_stats->tx_vport_broadcast_packets;
    vf_file_stats->tx_multicast += np_stats->tx_vport_multicast_packets;
    vf_file_stats->rx_dropped += msg->reps.stats_msg.rx_drop;
    vf_file_stats->tx_error += np_stats->tx_vport_mtu_drop_packets;
    vf_file_stats->rx_error += np_stats->rx_vport_mtu_drop_packets;
    kfree(np_stats);
    kfree(msg);
    return 0;
}

#define _sprintf(p, buf, format, arg...)				\
	((PAGE_SIZE - (int)(p - buf)) <= 0 ? 0 :			\
	scnprintf(p, PAGE_SIZE - (int)(p - buf), format, ## arg))

static ssize_t zxdh_vf_stats_show(struct zxdh_vf_obj *vf,
             struct zxdh_vf_attribute *attr, char *buf)
{
    char *p = buf;
    struct zxdh_vf_file_stats *vf_file_stats = NULL;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = vf->pf_dev->slot_id;
    pf_info.vport = vf->vport;
    vf_file_stats = kzalloc(sizeof(struct zxdh_vf_file_stats), GFP_KERNEL);
    if (vf_file_stats == NULL)
    {
        LOG_ERR("kzalloc(%lu, GFP_KERNEL) failed !", sizeof(struct zxdh_vf_file_stats));
        return -ENOMEM;
    }

    if(zxdh_vf_stats_update(pf_info, vf->vf_idx, vf->pf_dev, vf_file_stats) != 0)
    {
        kfree(vf_file_stats);
        return -ENOMEM;
    }

    p += _sprintf(p, buf, "tx_packets    : %llu\n", vf_file_stats->tx_packets);
    p += _sprintf(p, buf, "tx_bytes      : %llu\n", vf_file_stats->tx_bytes);
    p += _sprintf(p, buf, "rx_packets    : %llu\n", vf_file_stats->rx_packets);
    p += _sprintf(p, buf, "rx_bytes      : %llu\n", vf_file_stats->rx_bytes);
    p += _sprintf(p, buf, "rx_broadcast  : %llu\n", vf_file_stats->rx_broadcast);
    p += _sprintf(p, buf, "rx_multicast  : %llu\n", vf_file_stats->rx_multicast);
    p += _sprintf(p, buf, "tx_broadcast  : %llu\n", vf_file_stats->tx_broadcast);
    p += _sprintf(p, buf, "tx_multicast  : %llu\n", vf_file_stats->tx_multicast);
    p += _sprintf(p, buf, "rx_dropped    : %llu\n", vf_file_stats->rx_dropped);
    p += _sprintf(p, buf, "tx_error      : %llu\n", vf_file_stats->tx_error);
    p += _sprintf(p, buf, "rx_error      : %llu\n", vf_file_stats->rx_error);
    kfree(vf_file_stats);
    return (ssize_t)(p - buf);
}

static ssize_t zxdh_vf_stats_store(struct zxdh_vf_obj *vf,
             struct zxdh_vf_attribute *attr, const char *buf, size_t count)
{
    return -ENOTSUPP;
}

/* Our custom sysfs_ops that we will associate with our ktype later on */
static const struct sysfs_ops zxdh_group_sysfs_ops = {
    .show = zxdh_group_attr_show,
    .store = zxdh_group_attr_store,
};

static const struct sysfs_ops zxdh_vf_sysfs_ops = {
    .show = zxdh_vf_attr_show,
    .store = zxdh_vf_attr_store,
};

static const struct sysfs_ops zxdh_vf_meter_sysfs_ops = {
    .show = zxdh_vf_meter_attr_show,
    .store = zxdh_vf_meter_attr_store,
};

/*
 * The release function for our object.  This is REQUIRED by the kernel to
 * have.  We free the memory held in our object here.
 *
 * NEVER try to get away with just a "blank" release function to try to be
 * smarter than the kernel.  Turns out, no one ever is...
 */
/*static void zxdh_group_release(struct kobject *kobj)
{
    struct zxdh_group_obj *group;

    LOG_INFO("enter\n");
    group = to_zxdh_group_obj(kobj);
    kfree(group);
}

static void zxdh_vf_release(struct kobject *kobj)
{
    struct zxdh_vf_obj *vf;

    LOG_INFO("enter\n");
    vf = to_zxdh_vf_obj(kobj);
    kfree(vf);
}

static void zxdh_vf_meter_release(struct kobject *kobj)
{
    struct zxdh_vf_meter_obj *xps;

    LOG_INFO("enter\n");
    xps = to_zxdh_vf_meter_obj(kobj);
    kfree(xps);
}*/

#define _sprintf(p, buf, format, arg...)                \
    ((PAGE_SIZE - (int)(p - buf)) <= 0 ? 0 :            \
    scnprintf(p, PAGE_SIZE - (int)(p - buf), format, ## arg))

#ifdef ZXDH_PLCR_DEBUG
static ssize_t zxdh_burst_show(struct kobject *kobj,
            struct kobj_attribute *attr, char *buf)
{
    struct zxdh_pf_device *pf_dev;
    struct zxdh_sriov_sysfs *sriov;

    LOG_DEBUG("enter\n");
    sriov = container_of(attr, struct zxdh_sriov_sysfs, burst_attr);
    pf_dev = container_of(sriov, struct zxdh_pf_device, sriov);

    return sysfs_emit(buf,
            "the burst size = %dByte\n", pf_dev->plcr_table.burst_size);
}

static ssize_t zxdh_burst_store(struct kobject *kobj,
            struct kobj_attribute *attr, const char *buf, size_t count)
{
    int rtn = 0;
    uint32_t burst;
    struct zxdh_pf_device *pf_dev;
    struct zxdh_sriov_sysfs *sriov;

    LOG_DEBUG("enter\n");
    sriov = container_of(attr, struct zxdh_sriov_sysfs, burst_attr);
    pf_dev = container_of(sriov, struct zxdh_pf_device, sriov);

	rtn = kstrtoint(buf, 10, &burst);
    if (rtn)
		return rtn;

    /* When the burst changes, the rate limit of the specified flow needs
        to be reconfigured to update its burst.
       The scope of the burst includes the current PF and its associated VFs.
	   Write 0 to set the burst to the default value.*/

    pf_dev->plcr_table.burst_size = burst;

    return count;
}

static ssize_t zxdh_profile_stat_show(struct kobject *kobj,
            struct kobj_attribute *attr, char *buf)
{
    struct zxdh_sriov_sysfs *sriov = container_of(attr, struct zxdh_sriov_sysfs, profile_attr);
    struct zxdh_pf_device *pf_dev = container_of(sriov, struct zxdh_pf_device, sriov);
    struct zxdh_plcr_profile *profile;
    unsigned long index;
    uint32_t count = 0;
    E_PLCR_CAR_TYPE car_type;
    char *p = buf;

    LOG_DEBUG("enter\n");
    for (car_type = E_PLCR_CAR_A; car_type < E_PLCR_CAR_NUM; car_type ++)
    {
        count = 0;
        p += _sprintf(p, buf, "\n");
        p += _sprintf(p, buf, "car_type     : %d\n", car_type);
        p += _sprintf(p, buf, "profile_id   :");
        xa_for_each_range(&(pf_dev->plcr_table.plcr_profiles[car_type]), index,
                            profile, 0, gaudPlcrCarxProfileNum[car_type])
        {
            p += _sprintf(p, buf, " %d", profile->profile_id);
            count++;
        }
        p += _sprintf(p, buf, "\n");
        p += _sprintf(p, buf, "profiles_num : %d\n", count);    }
    return (ssize_t)(p - buf);
}

static ssize_t zxdh_all_vf_stats_store(struct kobject *kobj,
            struct kobj_attribute *attr, const char *buf, size_t count)
{
    return -ENOTSUPP;
}

static ssize_t zxdh_all_vf_stats_show(struct kobject *kobj,
            struct kobj_attribute *attr, char *buf)
{
    uint16_t vf_idx = 0;
    struct zxdh_vf_item *vf_item = NULL;
    struct zxdh_sriov_sysfs *sriov = container_of(attr, struct zxdh_sriov_sysfs, all_vf_stats_attr);
    struct zxdh_pf_device *pf_dev = container_of(sriov, struct zxdh_pf_device, sriov);
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    struct pci_dev *pdev = dh_dev->pdev;
    int num_vfs = pci_num_vf(pdev);
    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_vf_file_stats *vf_file_stats = NULL;
    char *p = buf;

    vf_file_stats = kzalloc(sizeof(struct zxdh_vf_file_stats), GFP_KERNEL);
    if (vf_file_stats == NULL)
    {
        LOG_ERR_DEV(dh_dev, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(struct zxdh_vf_file_stats));
        return -ENOMEM;
    }

    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        LOG_DEBUG_DEV(dh_dev, "current vf_idx %d\n", vf_idx);
        vf_item = zxdh_pf_get_vf_item(dh_dev, vf_idx);
        // 计算vf_idx的统计值（减去初始值）,添加到vf_file_stats总和中
        pf_info.slot = pf_dev->slot_id;
        pf_info.vport = vf_item->vport;
        if(zxdh_vf_stats_update(pf_info, vf_idx, pf_dev, vf_file_stats) != 0)
        {
            kfree(vf_file_stats);
            LOG_ERR_DEV(dh_dev, "zxdh_vf_stats_update failed, vf %d\n", vf_idx);
            return -ENOMEM;
        }
    }

    p += _sprintf(p, buf, "tx_packets    : %llu\n", vf_file_stats->tx_packets);
    p += _sprintf(p, buf, "tx_bytes      : %llu\n", vf_file_stats->tx_bytes);
    p += _sprintf(p, buf, "rx_packets    : %llu\n", vf_file_stats->rx_packets);
    p += _sprintf(p, buf, "rx_bytes      : %llu\n", vf_file_stats->rx_bytes);
    p += _sprintf(p, buf, "rx_broadcast  : %llu\n", vf_file_stats->rx_broadcast);
    p += _sprintf(p, buf, "rx_multicast  : %llu\n", vf_file_stats->rx_multicast);
    p += _sprintf(p, buf, "tx_broadcast  : %llu\n", vf_file_stats->tx_broadcast);
    p += _sprintf(p, buf, "tx_multicast  : %llu\n", vf_file_stats->tx_multicast);
    p += _sprintf(p, buf, "rx_dropped    : %llu\n", vf_file_stats->rx_dropped);
    p += _sprintf(p, buf, "tx_error      : %llu\n", vf_file_stats->tx_error);
    p += _sprintf(p, buf, "rx_error      : %llu\n", vf_file_stats->rx_error);
    kfree(vf_file_stats);
    return (ssize_t)(p - buf);
}
#endif

#ifdef ZXDH_PLCR_OPEN
static uint16_t zxdh_group_to_flowid(struct zxdh_group_obj *group, int32_t data_type)
{
    uint16_t flowid;
    uint16_t vport = group->pf_dev->vport;
    int32_t group_id = group->group_id;
    int32_t global_group_id;

    global_group_id = EPID(vport) * 128 + FUNC_NUM(vport) * 16 + group_id;
    flowid = (data_type == ZXDH_GROUP_TX_RATE) ?
                (global_group_id * 2 + 1) : (global_group_id * 2);

    return flowid;
}

static uint16_t zxdh_vport_to_flowid(uint16_t vport, int32_t req_type, int32_t direction)
{
    uint16_t vfid;
    uint16_t flowid;

    LOG_DEBUG("enter\n");
    vfid = VQM_VFID(vport);
    if (req_type == E_RATE_LIMIT_BYTE)
    {
        flowid = direction ? (vfid * 2 + 1) : (vfid * 2);
    }
    else if (req_type == E_RATE_LIMIT_PACKET)
    {
        flowid = direction ? (vfid * 2 + 1) : (vfid * 2) + PLCR_CAR_A_DPDK_FLOWID_OFFSET;
    }
    else
    {
        flowid = 0xffff;
    }

    return flowid;
}

static uint32_t zxdh_vf_meter_obj_to_flowid(struct zxdh_vf_meter_obj *xps)
{
    uint16_t vport;
    uint16_t vfid;
    uint16_t flowid;

    LOG_DEBUG("enter\n");
    vport = xps->vf_obj->vport;
    vfid = VQM_VFID(vport);
    flowid = IS_TX_METER(xps->meter_type) ? (vfid * 2 + 1) : (vfid * 2);

    return flowid;
}

static int zxdh_set_vf_group_rate_limit(struct zxdh_group_obj *group, int32_t direction, uint32_t max_rate)
{
    int rtn = 0;
    zxdh_plcr_rate_limit_paras rate_limit_paras;

    LOG_DEBUG("enter\n");
    if (!group->pf_dev->plcr_table.is_init)
        return 0;

    // en_priv = pf_dev_get_en_priv(group->pf_dev);
    // if (IS_ERR(en_priv))
    //     return PTR_ERR(en_priv);

    rate_limit_paras.req_type  = E_RATE_LIMIT_REQ_VF_GROUP_BYTE;
    rate_limit_paras.direction = direction; // rx:0, tx:1
    rate_limit_paras.mode      = E_RATE_LIMIT_BYTE;
    rate_limit_paras.max_rate  = max_rate;
    rate_limit_paras.min_rate  = PLCR_INVALID_PARAM;

    rate_limit_paras.queue_id  = PLCR_INVALID_PARAM;
    rate_limit_paras.vf_idx    = PLCR_INVALID_PARAM;
    rate_limit_paras.vfid      = PLCR_INVALID_PARAM;
    rate_limit_paras.group_id  = group->group_id;

    rtn = zxdh_plcr_unified_set_rate_limit(group->pf_dev, &rate_limit_paras);
    PLCR_COMM_ASSERT(rtn);

    return rtn;
}

static int zxdh_move_vf_to_group(struct zxdh_vf_obj *vf, struct zxdh_group_obj *group)
{
    int rtn = 0;
    zxdh_plcr_rate_limit_paras rate_limit_paras;

    LOG_DEBUG("enter\n");
    if (!vf->pf_dev->plcr_table.is_init)
        return 0;

    rate_limit_paras.req_type  = E_RATE_LIMIT_REQ_MOVE_VF_GROUP;
    rate_limit_paras.direction = E_RATE_LIMIT_RX;
    rate_limit_paras.mode      = PLCR_INVALID_PARAM;
    rate_limit_paras.max_rate  = PLCR_INVALID_PARAM;
    rate_limit_paras.min_rate  = PLCR_INVALID_PARAM;

    rate_limit_paras.queue_id  = PLCR_INVALID_PARAM;
    rate_limit_paras.vf_idx    = vf->vf_idx;
    rate_limit_paras.vfid      = PLCR_INVALID_PARAM;
    rate_limit_paras.group_id  = group->group_id;

    rtn = zxdh_plcr_unified_set_rate_limit(vf->pf_dev, &rate_limit_paras);
    PLCR_COMM_ASSERT(rtn);

#if 0
    //移动group，只需要调用一次统一限速接口，会同时将tx和rx进行映射
    rate_limit_paras.direction = E_RATE_LIMIT_TX;
    rtn = zxdh_plcr_unified_set_rate_limit(vf->pf_dev, &rate_limit_paras);
    PLCR_COMM_ASSERT(rtn);
#endif

    return rtn;
}
#endif

static ssize_t zxdh_group_max_rate_store(struct zxdh_group_obj *group,
             struct zxdh_group_attribute *attr, const char *buf,
             size_t count, int32_t direction)
{
    int32_t max_rate;
    // uint16_t flowid;
    int rtn = 0;
    LOG_DEBUG("enter\n");

    if (group == group->pf_dev->sriov.group_0)
        return -EPERM;

    rtn = kstrtoint(buf, 10, &max_rate);
    if (rtn < 0)
        return -EINVAL;
    LOG_DEBUG("max_%s_rate = %d\n", direction ? "tx" : "rx", max_rate);

    if ((direction ? group->max_tx_rate : group->max_rx_rate) == max_rate)
        return count;

    // flowid = zxdh_group_to_flowid(group, direction);

#ifdef ZXDH_PLCR_OPEN
    //调用限速统一接口，配置vf group限速
    rtn = zxdh_set_vf_group_rate_limit(group, direction, max_rate);
    if (rtn)
        return rtn;

    PLCR_LOG_INFO("The Max %s Rate of group%d has been set to %d Mbit/s\n",
            direction ? "Tx" : "Rx", group->group_id, max_rate);
#endif

    if (direction == ZXDH_GROUP_TX_RATE)
        group->max_tx_rate = max_rate;
    else
        group->max_rx_rate = max_rate;

    return count;
}

static ssize_t zxdh_group_max_rx_rate_show(struct zxdh_group_obj *group,
             struct zxdh_group_attribute *attr, char *buf)
{

    LOG_DEBUG("enter\n");
    return sysfs_emit(buf,
               "usage: write <Rate (Mbit/s)> to set VF group max rx rate\n");
}

static ssize_t zxdh_group_max_rx_rate_store(struct zxdh_group_obj *group,
             struct zxdh_group_attribute *attr, const char *buf, size_t count)
{
    return zxdh_group_max_rate_store(group, attr, buf, count, ZXDH_GROUP_RX_RATE);
}

/*
 * The "max_tx_rate" file where the .max_tx_rate variable is read from and written to.
 */
static ssize_t zxdh_group_max_tx_rate_show(struct zxdh_group_obj *group,
             struct zxdh_group_attribute *attr, char *buf)
{

    LOG_DEBUG("enter\n");
    return sysfs_emit(buf,
               "usage: write <Rate (Mbit/s)> to set VF group max tx rate\n");
}

static ssize_t zxdh_group_max_tx_rate_store(struct zxdh_group_obj *group,
             struct zxdh_group_attribute *attr, const char *buf, size_t count)
{
    return zxdh_group_max_rate_store(group, attr, buf, count, ZXDH_GROUP_TX_RATE);
}

static ssize_t zxdh_group_config_show(struct zxdh_group_obj *group,
             struct zxdh_group_attribute *attr, char *buf)
{
    //打印当前group配置信息
    char *p = buf;
    uint16_t vf_idx;
    int32_t num_vfs = group->pf_dev->num_vfs;
    struct zxdh_vf_obj *vf;

    LOG_DEBUG("enter\n");
    // if (!mutex_trylock(&esw->state_lock))
    //     return -EBUSY;

    p += _sprintf(p, buf, "GroupID   : %d\n", group->group_id);
    p += _sprintf(p, buf, "Num VFs   : %d\n", group->num_vfs);
    p += _sprintf(p, buf, "MaxTxRate : %d\n", group->max_tx_rate);
    p += _sprintf(p, buf, "MaxRxRate : %d\n", group->max_rx_rate);

    if (group->num_vfs)
    {
        p += _sprintf(p, buf, "VFs       : ");
        for (vf_idx = 0; vf_idx < num_vfs; vf_idx ++)
        {
            vf = group->pf_dev->sriov.vfs + vf_idx;
            if (vf->group == group)
                p += _sprintf(p, buf, "VF%d ", vf_idx);
        }
        p += _sprintf(p, buf, "\n");
    }

    // mutex_unlock(&esw->state_lock);

    return (ssize_t)(p - buf);
}

static ssize_t zxdh_group_config_store(struct zxdh_group_obj *group,
             struct zxdh_group_attribute *attr, const char *buf, size_t count)
{
    LOG_DEBUG("enter\n");
    return -ENOTSUPP;
}

static ssize_t zxdh_vf_config_show(struct zxdh_vf_obj *vf,
             struct zxdh_vf_attribute *attr, char *buf)
{
    //打印当前vf配置信息
#ifdef ZXDH_PLCR_OPEN
    struct zxdh_pf_device *pf_dev = vf->pf_dev;
    struct xarray *xa_bps_flow, *xa_pps_flow;
    struct zxdh_plcr_flow *flow_bps_rx, *flow_bps_tx, *flow_pps_rx, *flow_pps_tx;
    uint32_t id_bps_rx, id_bps_tx, id_pps_rx, id_pps_tx;
    E_RATE_LIMIT_MODE mode;
#endif
    char *p = buf;

    LOG_DEBUG("enter\n");
    // mutex_lock(&esw->state_lock);

#ifdef ZXDH_PLCR_OPEN
    xa_bps_flow = &(pf_dev->plcr_table.plcr_flows[E_PLCR_CAR_B]);
    zxdh_plcr_get_mode(pf_dev, vf->vport, &mode);

    id_bps_rx = zxdh_vport_to_flowid(vf->vport, E_RATE_LIMIT_BYTE, E_RATE_LIMIT_RX);
    id_bps_tx = zxdh_vport_to_flowid(vf->vport, E_RATE_LIMIT_BYTE, E_RATE_LIMIT_TX);
    flow_bps_rx = xa_load(xa_bps_flow, id_bps_rx);
    flow_bps_tx = xa_load(xa_bps_flow, id_bps_tx);
#endif

    p += _sprintf(p, buf, "VF          : VF%d\n", vf->vf_idx);
    p += _sprintf(p, buf, "RateGroup   : %d\n", vf->group->group_id);
#ifdef ZXDH_PLCR_OPEN
    p += _sprintf(p, buf, "VportQosMode: %d\n", mode);

    if (flow_bps_rx)
    {
        p += _sprintf(p, buf, "MinRxRate   : %dMbit/s\n", flow_bps_rx->min_rate);
        p += _sprintf(p, buf, "MaxRxRate   : %dMbit/s\n", flow_bps_rx->max_rate);
    }

    if (flow_bps_tx)
    {
        p += _sprintf(p, buf, "MinTxRate   : %dMbit/s\n", flow_bps_tx->min_rate);
        p += _sprintf(p, buf, "MaxTxRate   : %dMbit/s\n", flow_bps_tx->max_rate);
    }
    // if (mode == E_RATE_LIMIT_MODE0)
    // {

    // }
    // else if (mode == E_RATE_LIMIT_MODE1)
    // {

    // }
    if (mode == E_RATE_LIMIT_MODE2)
    {
        xa_pps_flow = &(pf_dev->plcr_table.plcr_flows[E_PLCR_CAR_A]);
        id_pps_rx = zxdh_vport_to_flowid(vf->vport, E_RATE_LIMIT_PACKET, E_RATE_LIMIT_RX);
        id_pps_tx = zxdh_vport_to_flowid(vf->vport, E_RATE_LIMIT_PACKET, E_RATE_LIMIT_TX);
        flow_pps_rx = xa_load(xa_pps_flow, id_pps_rx);
        flow_pps_tx = xa_load(xa_pps_flow, id_pps_tx);

        if (flow_pps_rx)
            p += _sprintf(p, buf, "MaxRxRate   : %dPackets/s\n", flow_pps_rx->max_rate);

        if (flow_pps_tx)
            p += _sprintf(p, buf, "MaxTxRate   : %dPackets/s\n", flow_pps_tx->min_rate);
    }

#endif

    // mutex_unlock(&esw->state_lock);

    return (ssize_t)(p - buf);
}

static ssize_t zxdh_vf_config_store(struct zxdh_vf_obj *vf,
             struct zxdh_vf_attribute *attr, const char *buf, size_t count)
{
    LOG_DEBUG("enter\n");
    return -ENOTSUPP;
}

static ssize_t zxdh_vf_group_show(struct zxdh_vf_obj *vf,
             struct zxdh_vf_attribute *attr, char *buf)
{
    LOG_DEBUG("enter\n");

    return sysfs_emit(buf,
            "usage: write <0-%d> to set VF vport group\n", ZXDH_MAX_VF_GROUP_OBJ_ID);
}

static ssize_t zxdh_vf_group_store(struct zxdh_vf_obj *vf,
             struct zxdh_vf_attribute *attr, const char *buf, size_t count)
{
    int32_t group_id;
    int rtn = 0;
    LOG_DEBUG("enter\n");

    rtn = kstrtoint(buf, 10, &group_id);
    if (rtn < 0)
        return -EINVAL;
    LOG_DEBUG("group = %d\n", group_id);

    rtn = zxdh_vf_update_sysfs_group(vf->pf_dev, vf, group_id);
    if (rtn)
    {
        LOG_ERR("zxdh_vf_update_sysfs_group failed\n");
        return rtn;
    }

    PLCR_LOG_INFO("VF%d has been moved to group%d\n", vf-> vf_idx, group_id);

    return count;
}

static ssize_t zxdh_vf_meter_rate_store(struct zxdh_vf_meter_obj *xps,
             struct zxdh_vf_meter_attribute *attr, const char *buf,
             size_t count, int32_t data_type)
{
#ifdef ZXDH_PLCR_OPEN
    struct zxdh_pf_device *pf_dev = xps->pf_dev;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    struct zxdh_vf_item *vf_item;
    struct xarray *xarray_flowid = &(pf_dev->plcr_table.plcr_flows[E_PLCR_CAR_B]);
    struct zxdh_plcr_flow *plcr_flow;
    zxdh_plcr_rate_limit_paras rate_limit_paras;
    uint32_t flowid;
    uint16_t vport;
    const char *direction;
#endif
    int32_t data;
    int rtn = 0;

    LOG_DEBUG("enter\n");
    rtn = kstrtoint(buf, 10, &data);
    if (rtn < 0)
        return -EINVAL;

#ifdef ZXDH_PLCR_OPEN
    if (!pf_dev->plcr_table.is_init)
        return count;
    vport = xps->vf_obj->vport;
    flowid = zxdh_vf_meter_obj_to_flowid(xps);
    plcr_flow = xa_load(xarray_flowid, flowid);

    //调用限速统一接口，配置vf限速
    if (IS_PPS_METER(xps->meter_type))
    {
        LOG_DEBUG_DEV(dh_dev, "max_rate = %d Packets/s\n", data);
        rate_limit_paras.req_type  = E_RATE_LIMIT_REQ_VF_PKT;
        rate_limit_paras.mode      = E_RATE_LIMIT_PACKET;
        rate_limit_paras.min_rate  = 0;
        rate_limit_paras.max_rate  = data;
    }
    else
    {
        rate_limit_paras.req_type  = E_RATE_LIMIT_REQ_VF_BYTE;
        rate_limit_paras.mode      = E_RATE_LIMIT_BYTE;

        if (data_type == ZXDH_VF_MIN_RATE)
        {
            LOG_DEBUG_DEV(dh_dev, "min_rate = %d Mbit/s\n", data);

            rate_limit_paras.min_rate  = data;
            if (plcr_flow)
                rate_limit_paras.max_rate  = max(plcr_flow->max_rate, rate_limit_paras.min_rate);
            else
                rate_limit_paras.max_rate  = data;
        }
        else if (data_type == ZXDH_VF_MAX_RATE)
        {
            LOG_DEBUG_DEV(dh_dev, "max_rate = %d Mbit/s\n", data);

            rate_limit_paras.max_rate  = data;
            if (plcr_flow)
                rate_limit_paras.min_rate  = min(plcr_flow->min_rate, rate_limit_paras.max_rate);
            else
                rate_limit_paras.min_rate  = 0;
        }
        else
            return -EINVAL;
    }

    rate_limit_paras.direction = IS_TX_METER(xps->meter_type) ? E_RATE_LIMIT_TX : E_RATE_LIMIT_RX;
    rate_limit_paras.queue_id  = PLCR_INVALID_PARAM;
    rate_limit_paras.vf_idx    = xps->vf_obj->vf_idx;
    rate_limit_paras.vfid      = PLCR_INVALID_PARAM;
    rate_limit_paras.group_id  = PLCR_INVALID_PARAM;

    rtn = zxdh_plcr_unified_set_rate_limit(pf_dev, &rate_limit_paras);
    PLCR_COMM_ASSERT(rtn);

    direction = IS_TX_METER(xps->meter_type) ? "Tx" : "Rx";
    if (IS_PPS_METER(xps->meter_type))
    {
        PLCR_LOG_INFO_DEV(dh_dev,
            "The Max %s Rate of VF%d has been set to %d Packets/s\n",
            direction, xps->vf_obj->vf_idx, data);
    }
    else
    {
        PLCR_LOG_INFO_DEV(dh_dev,
            "The Rate of VF%d has been set to: Min %s Rate: %d Mbit/s, Max %s Rate: %d Mbit/s\n",
            xps->vf_obj->vf_idx,
            direction, rate_limit_paras.min_rate,
            direction, rate_limit_paras.max_rate);
    }

    if (xps->meter_type == VF_METER_TX_BPS)
    {
        vf_item = &pf_dev->vf_item[rate_limit_paras.vf_idx];
        vf_item->min_tx_rate = rate_limit_paras.min_rate;
        vf_item->max_tx_rate = rate_limit_paras.max_rate;
    }
 
    if (E_RATE_LIMIT_TX == rate_limit_paras.direction)
    {
        //引入vqm vf限速，提高小流量限速精度;
        //条件1，4G以内满足配置能满足vqm限速周期全局共享,
        //条件2，每次配置新值需要将vqm vf限速清空再做配置
        rtn = zxdh_vqm_vf_set_rate_limit(pf_dev, rate_limit_paras.vfid , 0);
        PLCR_COMM_ASSERT(rtn);
        if (rate_limit_paras.max_rate < 4000)
        {
            rtn = zxdh_vqm_vf_set_rate_limit(pf_dev, rate_limit_paras.vfid , rate_limit_paras.max_rate);
            PLCR_COMM_ASSERT(rtn);
            PLCR_LOG_INFO_DEV(dh_dev, "The Rate of VF id:%d has been set to: Max Tx Rate: %dMbit/s in vqm\n",
                        rate_limit_paras.vfid , rate_limit_paras.max_rate);
        }
    }
    
#endif

    if (data_type == ZXDH_VF_MIN_RATE)
        xps->min_rate = data;
    else
        xps->max_rate = data;

    return count;
}

static ssize_t zxdh_vf_meter_min_rate_show(struct zxdh_vf_meter_obj *xps,
             struct zxdh_vf_meter_attribute *attr, char *buf)
{
    uint32_t meter_type = xps->meter_type;
    LOG_DEBUG("enter\n");

    return sysfs_emit(buf,
            "usage: write <Rate (Mbit/s)> to set VF %s min rate\n",
            IS_TX_METER(meter_type) ? "tx" : "rx");
}

static ssize_t zxdh_vf_meter_min_rate_store(struct zxdh_vf_meter_obj *xps,
             struct zxdh_vf_meter_attribute *attr, const char *buf, size_t count)
{
    return zxdh_vf_meter_rate_store(xps, attr, buf, count, ZXDH_VF_MIN_RATE);
}

static ssize_t zxdh_vf_meter_max_rate_show(struct zxdh_vf_meter_obj *xps,
             struct zxdh_vf_meter_attribute *attr, char *buf)
{
    uint32_t meter_type = xps->meter_type;
    LOG_DEBUG("enter\n");

    return sysfs_emit(buf,
            "usage: write <Rate (%s)> to set VF %s max rate\n",
            IS_PPS_METER(meter_type) ? "Packets/s" : "Mbit/s",
            IS_TX_METER(meter_type) ? "tx" : "rx");
}

static ssize_t zxdh_vf_meter_max_rate_store(struct zxdh_vf_meter_obj *xps,
             struct zxdh_vf_meter_attribute *attr, const char *buf, size_t count)
{
    return zxdh_vf_meter_rate_store(xps, attr, buf, count, ZXDH_VF_MAX_RATE);
}

#ifndef DEFAULT_GROUPS
#define ZXDH_RATE_GROUP_ATTR(_name) \
    static struct zxdh_group_attribute zxdh_group_##_name = \
    __ATTR(_name, 0644, zxdh_group_##_name##_show, zxdh_group_##_name##_store)

#define ZXDH_VF_ATTR(_name) \
    static struct zxdh_vf_attribute zxdh_vf_##_name = \
    __ATTR(_name, 0644, zxdh_vf_##_name##_show, zxdh_vf_##_name##_store)

#define ZXDH_VF_METER_ATTR(_name) \
    static struct zxdh_vf_meter_attribute zxdh_vf_meter_##_name = \
    __ATTR(_name, 0644, zxdh_vf_meter_##_name##_show, zxdh_vf_meter_##_name##_store)

/* Sysfs attributes cannot be world-writable. */
ZXDH_RATE_GROUP_ATTR(max_tx_rate);
ZXDH_RATE_GROUP_ATTR(max_rx_rate);
ZXDH_RATE_GROUP_ATTR(config);
ZXDH_VF_ATTR(config);
ZXDH_VF_ATTR(group);
ZXDH_VF_ATTR(stats);
ZXDH_VF_METER_ATTR(min_rate);
ZXDH_VF_METER_ATTR(max_rate);
#else
static struct zxdh_group_attribute zxdh_group_max_tx_rate __ro_after_init
    = __ATTR(max_tx_rate, 0644, zxdh_group_max_tx_rate_show, zxdh_group_max_tx_rate_store);
static struct zxdh_group_attribute zxdh_group_max_rx_rate __ro_after_init
    = __ATTR(max_rx_rate, 0644, zxdh_group_max_rx_rate_show, zxdh_group_max_rx_rate_store);
static struct zxdh_group_attribute zxdh_group_config __ro_after_init
    = __ATTR(config, 0644, zxdh_group_config_show, zxdh_group_config_store);

static struct zxdh_vf_attribute zxdh_vf_config __ro_after_init
    = __ATTR(config, 0644, zxdh_vf_config_show, zxdh_vf_config_store);
static struct zxdh_vf_attribute zxdh_vf_group __ro_after_init
    = __ATTR(group, 0644, zxdh_vf_group_show, zxdh_vf_group_store);
static struct zxdh_vf_attribute zxdh_vf_stats __ro_after_init
    = __ATTR(stats, 0644, zxdh_vf_stats_show, zxdh_vf_stats_store);

static struct zxdh_vf_meter_attribute zxdh_vf_meter_min_rate __ro_after_init
    = __ATTR(min_rate, 0644, zxdh_vf_meter_min_rate_show, zxdh_vf_meter_min_rate_store);
static struct zxdh_vf_meter_attribute zxdh_vf_meter_max_rate __ro_after_init
    = __ATTR(max_rate, 0644, zxdh_vf_meter_max_rate_show, zxdh_vf_meter_max_rate_store);
#endif

// static struct zxdh_group_attribute zxdh_group_max_tx_rate __ro_after_init
//     = __ATTR(max_tx_rate, 0200, NULL, zxdh_group_max_tx_rate_store);

// static struct zxdh_group_attribute zxdh_group_config __ro_after_init
//     = __ATTR(config, 0444, zxdh_group_config_show, NULL);

// static struct zxdh_vf_attribute zxdh_vf_group __ro_after_init
//     = __ATTR(group, 0644, zxdh_vf_group_show, zxdh_vf_group_store);

/*
 * Create a group of attributes so that we can create and destroy them all
 * at once.
 */

static struct attribute *zxdh_group_default_attrs[] = {
    &zxdh_group_max_tx_rate.attr,
    &zxdh_group_max_rx_rate.attr,
    &zxdh_group_config.attr,
    NULL,    /* need to NULL terminate the list of attributes */
};
#ifndef DEFAULT_GROUPS
ATTRIBUTE_GROUPS(zxdh_group_default);
#endif

static struct attribute *zxdh_vf_default_attrs[] = {
    &zxdh_vf_config.attr,
    &zxdh_vf_group.attr,
    &zxdh_vf_stats.attr,
    NULL,
};
#ifndef DEFAULT_GROUPS
ATTRIBUTE_GROUPS(zxdh_vf_default);
#endif

static struct attribute *zxdh_vf_meter_bps_attrs[] = {
    &zxdh_vf_meter_min_rate.attr,
    &zxdh_vf_meter_max_rate.attr,
    NULL,
};
#ifndef DEFAULT_GROUPS
ATTRIBUTE_GROUPS(zxdh_vf_meter_bps);
#endif

static struct attribute *zxdh_vf_meter_pps_attrs[] = {
    &zxdh_vf_meter_max_rate.attr,
    NULL,
};
#ifndef DEFAULT_GROUPS
ATTRIBUTE_GROUPS(zxdh_vf_meter_pps);
#endif

/*
 * Our own ktype for our kobjects.  Here we specify our sysfs ops, the
 * release function, and the set of default attributes we want created
 * whenever a kobject of this type is registered with the kernel.
 */
// static const struct kobj_type
static struct kobj_type zxdh_group_ktype = {
    .sysfs_ops = &zxdh_group_sysfs_ops,
    // .release = zxdh_group_release,
#ifndef DEFAULT_GROUPS
    .default_groups = zxdh_group_default_groups,
#else
    .default_attrs = zxdh_group_default_attrs,
# endif
};

// static const struct kobj_type
static struct kobj_type zxdh_vf_ktype = {
    .sysfs_ops = &zxdh_vf_sysfs_ops,
    // .release = zxdh_vf_release,
#ifndef DEFAULT_GROUPS
    .default_groups = zxdh_vf_default_groups,
#else
    .default_attrs = zxdh_vf_default_attrs,
# endif
};

static struct kobj_type zxdh_vf_meter_bps_ktype = {
    .sysfs_ops = &zxdh_vf_meter_sysfs_ops,
    // .release = zxdh_vf_meter_release,
#ifndef DEFAULT_GROUPS
    .default_groups = zxdh_vf_meter_bps_groups,
#else
    .default_attrs = zxdh_vf_meter_bps_attrs,
# endif
};

static struct kobj_type zxdh_vf_meter_pps_ktype = {
    .sysfs_ops = &zxdh_vf_meter_sysfs_ops,
    // .release = zxdh_vf_meter_release,
#ifndef DEFAULT_GROUPS
    .default_groups = zxdh_vf_meter_pps_groups,
#else
    .default_attrs = zxdh_vf_meter_pps_attrs,
# endif
};

static struct zxdh_group_obj *
zxdh_create_group_obj(struct zxdh_pf_device *pf_dev, int32_t group_id)
{
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    struct kobject *groups_obj = pf_dev->sriov.groups_obj;
    struct zxdh_group_obj *group;
    int rtn = 0;

    LOG_DEBUG("enter\n");
    /* allocate the memory for the whole object */
    group = kzalloc(sizeof(struct zxdh_group_obj), GFP_KERNEL);
    if (!group)
        return ERR_PTR(-ENOMEM);

    group->pf_dev = pf_dev;
    group->group_id = group_id;

    rtn = kobject_init_and_add(&group->kobj, &zxdh_group_ktype, groups_obj, "group%d", group_id);
    if (rtn)
    {
        LOG_INFO_DEV(dh_dev, "create group-%d kobject failed\n", group_id);
        kobject_put(&group->kobj);
        kfree(group);
        return ERR_PTR(rtn);
    }

    /* We are always responsible for sending the uevent that the kobject
     * was added to the system.
     */
    kobject_uevent(&group->kobj, KOBJ_ADD);

    list_add_tail(&group->list, &pf_dev->sriov.groups_head);

    init_completion(&group->free_group_comp);

    return group;
}

int zxdh_create_vf_obj(struct zxdh_pf_device *pf_dev, uint16_t vf_idx)
{
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    struct zxdh_vf_obj *vf_obj;
    int rtn;

    LOG_DEBUG_DEV(dh_dev, "enter\n");

    vf_obj = pf_dev->sriov.vfs + vf_idx;
    vf_obj->pf_dev = pf_dev;
    vf_obj->vport = pf_dev->vf_item[vf_idx].vport;
    vf_obj->vf_idx = vf_idx;
    vf_obj->group = pf_dev->sriov.group_0;

    LOG_DEBUG_DEV(dh_dev, "p_sriov_obj = %p, vport = %d\n", pf_dev->sriov.sriov_obj, pf_dev->vport);
    LOG_DEBUG_DEV(dh_dev, "vf_idx = %d, vport = %d, group_id = %d\n",\
            pf_dev->sriov.vfs[vf_idx].vf_idx, pf_dev->sriov.vfs[vf_idx].vport, pf_dev->sriov.vfs[vf_idx].group->group_id);

    rtn = kobject_init_and_add(&vf_obj->kobj, &zxdh_vf_ktype, pf_dev->sriov.sriov_obj,
                    "vf%d", vf_idx);
    if (rtn)
    {
        LOG_INFO_DEV(dh_dev, "create vf-%d kobject failed\n", vf_idx);
        kobject_put(&vf_obj->kobj);
        return -ENOMEM;
    }

    vf_obj->group->num_vfs ++;

    kobject_uevent(&vf_obj->kobj, KOBJ_ADD);

    return rtn;
}

static void zxdh_destroy_group_obj_work(struct work_struct *work)
{
	struct zxdh_group_work *group_work = container_of(work, struct zxdh_group_work, work);
	struct zxdh_group_obj *group_obj = group_work->group_obj;

    LOG_DEBUG("enter\n");
	kobject_put(&group_obj->kobj);
	complete_all(&group_obj->free_group_comp);
	kfree(group_work);
}

void zxdh_destroy_group_obj(struct zxdh_group_obj *group_obj)
{
	struct zxdh_group_work *group_work;

    LOG_DEBUG("enter\n");
    group_work = kzalloc(sizeof *group_work, GFP_ATOMIC);
    if (unlikely(NULL == group_work))
    {
        kobject_put(&group_obj->kobj);
        complete_all(&group_obj->free_group_comp);

        list_del(&group_obj->list);
        return;
    }

	INIT_WORK(&group_work->work, zxdh_destroy_group_obj_work);
	group_work->group_obj = group_obj;
	queue_work(system_wq, &group_work->work);

    list_del(&group_obj->list);
}

void zxdh_destroy_vf_obj(struct zxdh_pf_device *pf_dev, uint16_t vf_idx)
{
    struct zxdh_vf_obj *vf_obj;

    LOG_DEBUG("enter\n");
    vf_obj = pf_dev->sriov.vfs + vf_idx;
    kobject_put(&vf_obj->kobj);
}


/**********************************************
**********************************************/

static struct zxdh_group_obj *
zxdh_find_sysfs_group(struct zxdh_pf_device *pf_dev, int32_t group_id)
{
    struct zxdh_group_obj *group;

    list_for_each_entry(group, &pf_dev->sriov.groups_head, list)
    {
        if (group->group_id == group_id)
            return group;
    }

    return NULL;
}

int zxdh_vf_update_sysfs_group(struct zxdh_pf_device *pf_dev, struct zxdh_vf_obj *vf, int32_t group_id)
{
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    struct zxdh_group_obj *curr_group;
    struct zxdh_group_obj *new_group;
    int rtn = 0;
#ifdef ZXDH_PLCR_OPEN
    struct xarray *xarray_flowid;
    struct zxdh_plcr_flow *plcr_rx_flow;
    struct zxdh_plcr_flow *plcr_tx_flow;
    int16_t curr_rx_id;
    int16_t curr_tx_id;
#endif

    // mutex_lock(&esw->state_lock);

    curr_group = vf->group;
    if (curr_group && curr_group->group_id == group_id)
    {
        LOG_ERR_DEV(dh_dev, "VF is already in the group%d\n", group_id);
        goto out;
    }

    if (group_id)
    {
        new_group = zxdh_find_sysfs_group(pf_dev, group_id);
        if (!new_group)
        {
            new_group = zxdh_create_group_obj(pf_dev, group_id);
            if (IS_ERR(new_group))
            {
                rtn = PTR_ERR(new_group);
                LOG_ERR_DEV(dh_dev, "create new sysfs group-%d failed (%d)\n", group_id, rtn);
                goto out;
            }
        }
    }
    else
    {
        new_group = pf_dev->sriov.group_0;
    }

#ifdef ZXDH_PLCR_OPEN
    //调用限速统一接口，映射vf到目标group
    rtn = zxdh_move_vf_to_group(vf, new_group);
    if (rtn)
    {
        PLCR_LOG_ERR("failed and rtn=0x%x\n", rtn)
        goto err_update;
    }
#endif

    vf->group = new_group;
    new_group->num_vfs++;
    if (!curr_group)
        goto out;
    curr_group->num_vfs--;

    if (curr_group != pf_dev->sriov.group_0 && !curr_group->num_vfs)
    {
        zxdh_destroy_group_obj(curr_group);

#ifdef ZXDH_PLCR_OPEN
        //限速统一接口，移除当前carc flow限速
        curr_rx_id = zxdh_group_to_flowid(curr_group, ZXDH_GROUP_RX_RATE);
        curr_tx_id = zxdh_group_to_flowid(curr_group, ZXDH_GROUP_TX_RATE);

        xarray_flowid = &(pf_dev->plcr_table.plcr_flows[E_PLCR_CAR_C]);
        plcr_rx_flow = xa_load(xarray_flowid, curr_rx_id);
        plcr_tx_flow = xa_load(xarray_flowid, curr_tx_id);

        if (xa_load(xarray_flowid, curr_rx_id))
            rtn = zxdh_set_vf_group_rate_limit(curr_group, ZXDH_GROUP_RX_RATE, 0);
        if (xa_load(xarray_flowid, curr_tx_id))
            rtn = zxdh_set_vf_group_rate_limit(curr_group, ZXDH_GROUP_TX_RATE, 0);
#endif

        wait_for_completion(&curr_group->free_group_comp);
	    kfree(curr_group);
    }
    goto out;

#ifdef ZXDH_PLCR_OPEN
err_update:
    if (new_group != pf_dev->sriov.group_0 && !new_group->num_vfs)
    {
        zxdh_destroy_group_obj(new_group);

        wait_for_completion(&new_group->free_group_comp);
	    kfree(new_group);
    }
#endif
out:
    // mutex_unlock(&esw->state_lock);
    return rtn;
}

static int zxdh_creat_vf_meter_obj(struct zxdh_vf_obj *vf,
                        struct zxdh_vf_meters *meters,
                        uint32_t meter_type)
{
    struct zxdh_vf_meter_obj *xps;
    struct kobject *parent;
    struct kobj_type *ktype;
    const char *name;
    int rtn;

    if (meter_type >= VF_METER_TYPE_NUM)
        return -EINVAL;

    xps = &meters->xps[meter_type];

    if (IS_TX_METER(meter_type))
        parent = meters->tx_obj;
    else
        parent = meters->rx_obj;

    if (IS_PPS_METER(meter_type))
    {
        ktype = &zxdh_vf_meter_pps_ktype;
        name = "pps";
    }
    else
    {
        ktype = &zxdh_vf_meter_bps_ktype;
        name = "bps";
    }

    rtn = kobject_init_and_add(&xps->kobj, ktype, parent, name);
    if (rtn)
    {
        kobject_put(&xps->kobj);
        return rtn;
    }

    xps->pf_dev = vf->pf_dev;
    xps->vf_obj = vf;
    xps->meter_type = meter_type;

    return 0;
}

int zxdh_create_vf_meters_sysfs(struct zxdh_pf_device *pf_dev, uint16_t vf_idx)
{
    int rtn = 0;
    struct zxdh_vf_obj *vf;
    struct zxdh_vf_meters *meters;

    LOG_DEBUG("enter\n");

    vf = pf_dev->sriov.vfs + vf_idx;

    meters = kzalloc(sizeof(struct zxdh_vf_meters), GFP_KERNEL);
    if (!meters)
    {
        return -ENOMEM;
    }

    meters->kobj = kobject_create_and_add("meters", &vf->kobj);
    if (!meters->kobj)
    {
        rtn = -EINVAL;
        goto err_vf_meters;
    }

    meters->rx_obj = kobject_create_and_add("rx", meters->kobj);
    if (!meters->rx_obj)
    {
        rtn = -EINVAL;
        goto err_vf_meters;
    }

    meters->tx_obj = kobject_create_and_add("tx", meters->kobj);
    if (!meters->tx_obj)
    {
        rtn = -EINVAL;
        goto err_vf_meters;
    }

    rtn = zxdh_creat_vf_meter_obj(vf, meters, VF_METER_RX_BPS);
    if (rtn)
        goto err_vf_meters;

    rtn = zxdh_creat_vf_meter_obj(vf, meters, VF_METER_RX_PPS);
    if (rtn)
        goto err_put_xps_0;

    rtn = zxdh_creat_vf_meter_obj(vf, meters, VF_METER_TX_BPS);
    if (rtn)
        goto err_put_xps_1;

    rtn = zxdh_creat_vf_meter_obj(vf, meters, VF_METER_TX_PPS);
    if (rtn)
        goto err_put_xps_2;

    vf->meters = meters;

    return 0;

err_put_xps_2:
    kobject_put(&meters->xps[VF_METER_TX_BPS].kobj);
err_put_xps_1:
    kobject_put(&meters->xps[VF_METER_RX_PPS].kobj);
err_put_xps_0:
    kobject_put(&meters->xps[VF_METER_RX_BPS].kobj);
err_vf_meters:
    kobject_put(meters->rx_obj);
    kobject_put(meters->tx_obj);
    kobject_put(meters->kobj);

    kfree(meters);

    return rtn;
}

static void zxdh_destroy_vf_meters_sysfs(struct zxdh_pf_device *pf_dev, uint16_t vf_idx)
{
    struct zxdh_vf_obj *vf;
    struct zxdh_vf_meters *meters;
    uint32_t meter_type;

    LOG_DEBUG("enter\n");

    vf = pf_dev->sriov.vfs + vf_idx;
    meters = vf->meters;
    if (!meters)
        return;

    //限速统一接口，移除当前vf所有限速 todo:en层的vf remove应该已经执行了该操作

    for (meter_type = 0; meter_type < 4; meter_type++)
        kobject_put(&meters->xps[meter_type].kobj);

    kobject_put(meters->rx_obj);
    kobject_put(meters->tx_obj);
    kobject_put(meters->kobj);

    kfree(meters);
}

int zxdh_create_vfs_sysfs(struct dh_core_dev *dev, int32_t num_vfs)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    int rtn = 0;
    uint16_t vf_idx;

    LOG_DEBUG_DEV(dev, "enter\n");
    pf_dev->sriov.vfs = kcalloc(num_vfs, sizeof(struct zxdh_vf_obj), GFP_KERNEL);
    if (!pf_dev->sriov.vfs)
    {
        LOG_ERR_DEV(dev, "kcalloc vfs failed\n");
        return -ENOMEM;
    }

    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        rtn = zxdh_create_vf_obj(pf_dev, vf_idx);
        if (rtn)
        {
            LOG_ERR_DEV(dev, "zxdh_create_vf_obj failed\n");
            goto err_vf;
        }

        rtn = zxdh_create_vf_meters_sysfs(pf_dev, vf_idx);
        if (rtn)
        {
            zxdh_destroy_vf_obj(pf_dev, vf_idx);
            LOG_ERR_DEV(dev, "zxdh_create_vf_meters_sysfs failed\n");
            goto err_vf;
        }
    }

    return 0;

err_vf:
    for (; vf_idx > 0; --vf_idx)
    {
        zxdh_destroy_vf_meters_sysfs(pf_dev, vf_idx);

        zxdh_destroy_vf_obj(pf_dev, vf_idx);
    }

    kfree(pf_dev->sriov.vfs);
    pf_dev->sriov.vfs = NULL;

    return rtn;
}

void zxdh_destroy_vfs_sysfs(struct dh_core_dev *dev, int32_t num_vfs)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    struct zxdh_vf_obj *vf;
    uint16_t vf_idx;

    LOG_DEBUG("enter\n");
    if (!num_vfs || !pf_dev->sriov.vfs)
        return;

    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        pf_dev->vf_item[vf_idx].min_tx_rate = 0;
        pf_dev->vf_item[vf_idx].max_tx_rate = 0;

        vf = pf_dev->sriov.vfs + vf_idx;

        if (vf->group != pf_dev->sriov.group_0)
            zxdh_vf_update_sysfs_group(pf_dev, vf, 0);

        zxdh_destroy_vf_meters_sysfs(pf_dev, vf_idx);

        zxdh_destroy_vf_obj(pf_dev, vf_idx);
    }
    kfree(pf_dev->sriov.vfs);
    pf_dev->sriov.vfs = NULL;
}

void zxdh_cleanup_sysfs_group(struct zxdh_pf_device *pf_dev)
{
    struct zxdh_group_obj *group, *tmp;

    LOG_DEBUG("enter\n");
    list_for_each_entry_safe(group, tmp, &pf_dev->sriov.groups_head, list)
    {
        list_del(&group->list);
        kfree(group);
    }
}

#ifdef ZXDH_PLCR_DEBUG
int zxdh_sriov_attr_create(struct zxdh_pf_device *pf_dev)
{
    int rtn;
    struct zxdh_sriov_sysfs *sriov = &pf_dev->sriov;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    LOG_DEBUG_DEV(dh_dev, "enter\n");

	sriov->burst_attr.attr.name = "burst";
	sriov->burst_attr.attr.mode = 0644;
	sriov->burst_attr.show = zxdh_burst_show;
	sriov->burst_attr.store = zxdh_burst_store;
    rtn = sysfs_create_file(sriov->sriov_obj, &sriov->burst_attr.attr);
    if (rtn)
    {
        LOG_ERR_DEV(dh_dev, "burst sysfs_create_file failed!")
        return rtn;
    }

	sriov->profile_attr.attr.name = "profiles_stat";
	sriov->profile_attr.attr.mode = 0444;
	sriov->profile_attr.show = zxdh_profile_stat_show;
	sriov->profile_attr.store = NULL;
    rtn = sysfs_create_file(sriov->sriov_obj, &sriov->profile_attr.attr);
    if (rtn)
    {
        LOG_ERR_DEV(dh_dev, "profiles_stat sysfs_create_file failed!")
        goto err_profile_attr;
    }

    sriov->all_vf_stats_attr.attr.name = "all_vf_stats";
    sriov->all_vf_stats_attr.attr.mode = 0644;
    sriov->all_vf_stats_attr.show = zxdh_all_vf_stats_show;
    sriov->all_vf_stats_attr.store = zxdh_all_vf_stats_store;
    rtn = sysfs_create_file(sriov->sriov_obj, &sriov->all_vf_stats_attr.attr);
    if (rtn)
    {
        LOG_ERR_DEV(dh_dev, "all_vf_stats sysfs_create_file failed!")
        goto err_all_vf_stats_attr;
    }

    return rtn;
err_all_vf_stats_attr:
    sysfs_remove_file(sriov->sriov_obj, &sriov->profile_attr.attr);
err_profile_attr:
    sysfs_remove_file(sriov->sriov_obj, &sriov->burst_attr.attr);
    return rtn;
}

void zxdh_sriov_attr_remove(struct zxdh_pf_device *pf_dev)
{
    struct zxdh_sriov_sysfs *sriov = &pf_dev->sriov;

    LOG_DEBUG("enter\n");
    sysfs_remove_file(sriov->sriov_obj, &sriov->all_vf_stats_attr.attr);
    sysfs_remove_file(sriov->sriov_obj, &sriov->profile_attr.attr);
    sysfs_remove_file(sriov->sriov_obj, &sriov->burst_attr.attr);
}
#endif

int zxdh_sriov_sysfs_init(struct dh_core_dev *dev)
{
    struct device *device = dev->device;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    int rtn = 0;

    if (dev->coredev_type != DH_COREDEV_PF)
        return rtn;

    pf_dev->sriov.sriov_obj = kobject_create_and_add("sriov", &device->kobj);
    if (!pf_dev->sriov.sriov_obj)
    {
        LOG_ERR_DEV(dev, "zxdh create sriov sysfs failed (%d)\n", -ENOMEM);
        return -ENOMEM;
    }

#ifdef ZXDH_PLCR_DEBUG
    rtn = zxdh_sriov_attr_create(pf_dev);
    if (rtn)
    {
        LOG_ERR_DEV(dev, "zxdh_sriov_attr_create failed (%d)\n", rtn);
        goto err_attr;
    }
#endif

    pf_dev->sriov.groups_obj = kobject_create_and_add("groups", pf_dev->sriov.sriov_obj);
    if (!pf_dev->sriov.groups_obj)
    {
        LOG_ERR_DEV(dev, "zxdh create groups sysfs failed (%d)\n", -ENOMEM);
        rtn = -ENOMEM;
        goto err_groups;
    }

    INIT_LIST_HEAD(&pf_dev->sriov.groups_head);
    pf_dev->sriov.group_0 = zxdh_create_group_obj(pf_dev, ZXDH_DEFAULT_VF_GROUP_ID);
    if (IS_ERR(pf_dev->sriov.group_0))
    {
        LOG_ERR_DEV(dev, "zxdh create rate group 0 failed (%ld)\n", PTR_ERR(pf_dev->sriov.group_0));
        rtn = PTR_ERR(pf_dev->sriov.group_0);
        goto err_group0;
    }

    LOG_DEBUG_DEV(dev, "p_sriov_obj = %p, vport = 0x%x\n", pf_dev->sriov.sriov_obj, pf_dev->vport);

    return rtn;

err_group0:
    kobject_put(pf_dev->sriov.groups_obj);
    pf_dev->sriov.groups_obj = NULL;
err_groups:
#ifdef ZXDH_PLCR_DEBUG
    zxdh_sriov_attr_remove(pf_dev);
err_attr:
#endif
    kobject_put(pf_dev->sriov.sriov_obj);
    pf_dev->sriov.sriov_obj = NULL;

    return rtn;
}

void zxdh_sriov_sysfs_exit(struct dh_core_dev *dev)
{
    // struct device *device = &dev->pdev->dev;
    struct pci_dev *pdev = dev->pdev;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    int32_t num_vfs = pci_num_vf(pdev);

    if (dev->coredev_type != DH_COREDEV_PF)
        return;

    LOG_DEBUG_DEV(dev, "enter\n");

    /*对于未通过sriov_config手动删除vf，直接进行驱动卸载的情况，此处需要进行检查*/
    zxdh_destroy_vfs_sysfs(dev, num_vfs);

    zxdh_destroy_group_obj(pf_dev->sriov.group_0);
    wait_for_completion(&pf_dev->sriov.group_0->free_group_comp);
	kfree(pf_dev->sriov.group_0);

    zxdh_cleanup_sysfs_group(pf_dev);

    kobject_put(pf_dev->sriov.groups_obj);
    pf_dev->sriov.groups_obj = NULL;
#ifdef ZXDH_PLCR_DEBUG
    zxdh_sriov_attr_remove(pf_dev);
#endif
    kobject_put(pf_dev->sriov.sriov_obj);
    pf_dev->sriov.sriov_obj = NULL;
}
