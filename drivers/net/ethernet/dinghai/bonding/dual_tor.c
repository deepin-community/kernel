#include "dual_tor.h"

static atomic_t user_heart_lose = ATOMIC_INIT(0);
static struct timer_list heartbeat_timer;

static void heartbeat_timer_callback(struct timer_list *t)
{
    int value = 0;

    value = atomic_inc_return(&user_heart_lose);
    if (value > HEARTBEAT_THRESHOLD)
    {
        atomic_set(&user_heart_lose, HEARTBEAT_THRESHOLD);
    }

    mod_timer(&heartbeat_timer, jiffies + msecs_to_jiffies(1));
}

void rdma_mac_fomat_convert(uint64_t rdma_format_mac, uint8_t *mac_local)
{
    int i = 0;
    uint8_t *rdma_mac = (uint8_t *)&rdma_format_mac;

    for (i = 0; i < 6; i++)
    {
        mac_local[i] = rdma_mac[5 - i];
    }
}

void ip_to_string(const uint32_t ip[4], char *ip_str, size_t max_len, bool is_ipv4)
{
    int ret = 0;

    if (!ip || !ip_str) {
        return;
    }

    if (is_ipv4)
    {
        ret = zte_snprintf_s(ip_str, max_len, "%u.%u.%u.%u",
                 (ip[3] >> 24) & 0xFF, (ip[3] >> 16) & 0xFF,
                 (ip[3] >> 8) & 0xFF, ip[3] & 0xFF);
        if (ret < 0) {
            return;
        }
    }
    else
    {
        ret = zte_snprintf_s(ip_str, max_len,
                 "%x:%x:%x:%x:%x:%x:%x:%x",
                 (ip[0] >> 16) & 0xFFFF, ip[0] & 0xFFFF,
                 (ip[1] >> 16) & 0xFFFF, ip[1] & 0xFFFF,
                 (ip[2] >> 16) & 0xFFFF, ip[2] & 0xFFFF,
                 (ip[3] >> 16) & 0xFFFF, ip[3] & 0xFFFF);
        if (ret < 0) {
            return;
        }
    }
}

const char *ip_state_to_str(enum ip_state state)
{
    switch (state)
    {
        case STATE_NEW: return "STATE_NEW";
        case STATE_NOTIFY: return "STATE_NOTIFY";
        case STATE_DONE: return "STATE_DONE";
        case STATE_DELETING: return "STATE_DELETING";
        case STATE_DELETED: return "STATE_DELETED";
        case STATE_ERROR: return "STATE_ERROR";
        default: return "UNKNOWN_STATE";
    }
}

bool is_msg_list_over_limit(struct list_head *list, int max_limit)
{
    int count = 0;
    struct list_head *pos;

    list_for_each(pos, list)
    {
        if (++count > max_limit)
            return true;
    }
    return false;
}

void format_mac(const uint8_t *mac, char *buf)
{
    int ret = 0;
    if (mac == NULL || buf == NULL)
        return;

    ret = zte_snprintf_s(buf, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    if (ret < 0) {
        return;
    }
}
/*************************以上为通用函数*********************************/

/* 一级bond设备表*/
static struct bond_devs_rht_st bond_devs_rht_info = {0};

/* 获取一级表*/
struct bond_devs_rht_st *bond_devs_rht_st_get(void)
{
    return &bond_devs_rht_info;
}

void bond_devs_lock(void)
{
    struct bond_devs_rht_st *devs_st = bond_devs_rht_st_get();
    mutex_lock(&devs_st->devs_lock);
    PSN_LOG_DEBUG("glb_lock.\n");
}

void bond_devs_unlock(void)
{
    struct bond_devs_rht_st *devs_st = bond_devs_rht_st_get();
    PSN_LOG_DEBUG("glb_unlock.\n");
    mutex_unlock(&devs_st->devs_lock);
}

int bond_devs_rht_init(void)
{
    int ret = 0;
    struct bond_devs_rht_st *devs_st = bond_devs_rht_st_get();
    struct rhashtable_params *params = &devs_st->bond_devs_rht_params;

    /* 初始化参数结构体*/
    params->key_len = IFNAME_MAX_SIZE;
    params->key_offset = offsetof(struct bond_dev_hash_item, ifname);
    params->head_offset = offsetof(struct bond_dev_hash_item, node);
    params->automatic_shrinking = true;

    ret = rhashtable_init(&devs_st->bond_devs_rht, params);
    if (ret) {
        PSN_LOG_ERR("Failed to initialize rhashtable\n");
        return ret;
    }
#ifdef CGS_V5_693
    PSN_LOG_DEBUG("key_len: %zu, key_offset: %zu, head_offset:%zu.\n", \
           params->key_len, params->key_offset, params->head_offset);
#else
    PSN_LOG_DEBUG("key_len: %u, key_offset: %u, head_offset:%u.\n", \
           params->key_len, params->key_offset, params->head_offset);
#endif
    mutex_init(&devs_st->devs_lock);
    return 0;
}

/* 查询一级表*/
struct bond_dev_hash_item *bond_dev_get_by_ifname(char *ifname)
{
    uint8_t ifname_key[IFNAME_MAX_SIZE] = {0};
    struct bond_devs_rht_st *devs_st = NULL;

    devs_st = bond_devs_rht_st_get();
    if (NULL == ifname)
    {
        return NULL;
    }
    if (NULL != ifname && zte_strlen_s(ifname) >= IFNAME_MAX_SIZE)
    {
        PSN_LOG_ERR("invalid ifname: %s.\n", ifname);
        return NULL;
    }

    zte_strncpy_s((char *)ifname_key, ifname, IFNAME_MAX_SIZE - 1);
    return rhashtable_lookup_fast(&devs_st->bond_devs_rht, ifname_key, devs_st->bond_devs_rht_params);
}

void bond_dev_info_destroy(struct bond_dev_info *dev_info);
/* 删除单个条目, 要求外部加锁*/
int bond_dev_del_by_ifname(char *ifname)
{
    struct bond_dev_hash_item *item_to_check = NULL;
    struct bond_devs_rht_st *devs_st = bond_devs_rht_st_get();

    item_to_check = bond_dev_get_by_ifname(ifname);
    if (!item_to_check)
    {
        PSN_LOG_DEBUG("ifname %s already del while try to dellete!\n", ifname);
        return -1;
    }

    rhashtable_remove_fast(&devs_st->bond_devs_rht, &item_to_check->node, devs_st->bond_devs_rht_params);
    /* TODO: 在这里清除掉每一个子项的列表*/
    bond_dev_info_destroy(&item_to_check->dev_info);
    if (item_to_check) {
        PSN_LOG_DEBUG("clear dev:%s.\n", item_to_check->ifname);
        kfree(item_to_check);
    }
    return 0;
}

void bond_devs_rht_destory(void)
{
    struct rhashtable_iter iter;
    struct bond_dev_hash_item *entry = NULL;
    struct bond_devs_rht_st *devs_st = bond_devs_rht_st_get();

    bond_devs_lock();
    rhashtable_walk_enter(&devs_st->bond_devs_rht, &iter);
    rhashtable_walk_start(&iter);
    while (true)
    {
        entry = rhashtable_walk_next(&iter);
        if (entry == NULL) {
            break;
        }

        if (IS_ERR(entry)) {
            if (PTR_ERR(entry) == -EAGAIN) {
                /* 如果是 -EAGAIN 错误，继续遍历 */
                continue;
            } else {
                /* 其他错误，停止遍历 */
                PSN_LOG_DEBUG("Error during rhashtable walk: %ld\n", PTR_ERR(entry));
                break;
            }
        }

        bond_dev_del_by_ifname(entry->ifname);
    }
    rhashtable_walk_stop(&iter);
    rhashtable_walk_exit(&iter);
    bond_devs_unlock();

    rhashtable_destroy(&devs_st->bond_devs_rht);
}

int bond_devs_find_by_linked_fid(uint32_t linked_fid, char *bond_name)
{
    int idx = 0;
    int ret = -1;
    struct rhashtable_iter iter;
    struct bond_dev_hash_item *entry = NULL;
    struct bond_devs_rht_st *devs_st = bond_devs_rht_st_get();
    char link_pf_name[IFNAME_MAX_SIZE] = {0};
    struct bond_ports_info *ports_desc = NULL;

    PF_FID_GET(linked_fid);

    bond_devs_lock();
    rhashtable_walk_enter(&devs_st->bond_devs_rht, &iter);
    rhashtable_walk_start(&iter);
    while (true) {
        entry = rhashtable_walk_next(&iter);
        if (entry == NULL) {
            break;
        }

        if (IS_ERR(entry)) {
            if (PTR_ERR(entry) == -EAGAIN) {
                continue;
            } else {
                PSN_LOG_DEBUG("Error during rhashtable walk: %ld\n", PTR_ERR(entry));
                break;
            }
        }

        /* rdma业务提供vf设备fid, 推送出属于哪一个bond*/
        ports_desc = &entry->dev_info.ports_desc;
        for (idx = 0; idx < ports_desc->slaves_num; idx++)
        {
            if (ports_desc->slave_devs[idx].fid == linked_fid)
            {
                zte_memcpy_s(bond_name, entry->ifname, sizeof(entry->ifname));
                ret = 0;
                goto out;
            }
        }
    }

out:
    rhashtable_walk_stop(&iter);
    rhashtable_walk_exit(&iter);
    bond_devs_unlock();
    PSN_LOG_DEBUG("link_name: 0x%x conresbonding to link_pf_name: %s.\n", linked_fid, link_pf_name);
    return ret;
}

int psn_update_local_bond_state(char *ifname, struct bond_ports_info *ports_desc_ext)
{
    int idx = 0;
    struct bond_dev_hash_item *bond_dev = NULL;
    struct bond_ports_info *ports_desc_loc = NULL;
    struct slave_dev *psn_slave_loc = NULL;

    bond_devs_lock();
    bond_dev = bond_dev_get_by_ifname(ifname);
    if (!bond_dev)
    {
        bond_devs_unlock();
        return -1;
    }

    ports_desc_loc = &bond_dev->dev_info.ports_desc;
    for (idx = 0; idx < ports_desc_loc->slaves_num; idx++)
    {
        psn_slave_loc = &ports_desc_loc->slave_devs[idx];
        if (!ports_desc_ext->slave_devs[idx].is_enable)
        {
            psn_slave_loc->is_update = 0;
            /* 如果根本没有扫描到， 就不更新该端口*/
            continue;
        }
        psn_slave_loc->is_update = (psn_slave_loc->link_state == ports_desc_ext->slave_devs[idx].link_state)? 0 : 1;
        psn_slave_loc->link_state = (ports_desc_ext->slave_devs[idx].link_state == 0? BOND_STATE_DOWN : BOND_STATE_UP);
        PSN_LOG_INFO("bond-%s has %u ports, idx-%d np_port-%u update to %u.\n",ifname,
                                                                 ports_desc_loc->slaves_num,
                                                                 idx, psn_slave_loc->np_port, psn_slave_loc->link_state);
    }
    bond_devs_unlock();
    return 0;
}

/*********************以上为一级表操作， 下方为二级表操作***********************/
void psn_mutex_lock(struct mutex *lock)
{
    mutex_lock(lock);
    PSN_LOG_DEBUG("lock.\n");
}

void psn_mutex_unlock(struct mutex *lock)
{
    PSN_LOG_DEBUG("unlock.\n");
    mutex_unlock(lock);
}

int bond_dev_info_init(struct bond_dev_info *dev_info)
{
    mutex_init(&dev_info->remote_ip_lock);

    dev_info->ip_hash_params = (struct rhashtable_params)
    {
        .key_len = sizeof(struct ip_info),
        .key_offset = offsetof(struct remote_ip_hash_item, ip_pair),
        .head_offset = offsetof(struct remote_ip_hash_item, node),
        .automatic_shrinking = true,
    };

    if (rhashtable_init(&dev_info->remote_ip_hash_list, &dev_info->ip_hash_params))
    {
        PSN_LOG_ERR("Failed to initialize remote_ip_hash_list.\n");
        return -1;
    }
    return 0;
}

/* 插入单个ip条目*/
int bond_dev_insert_by_ifname(char *ifname, struct bond_ports_info *port_desc_ext)
{
    int ret = 0;
    struct bond_dev_hash_item *item_to_check = NULL;
    struct bond_dev_hash_item *item_to_insert = NULL;
    struct bond_devs_rht_st *devs_st = bond_devs_rht_st_get();
    struct bond_ports_info *port_desc_loc = NULL;

    bond_devs_lock();
    item_to_check = bond_dev_get_by_ifname(ifname);
    if (item_to_check)
    {
        PSN_LOG_DEBUG("ifname: %s already insert!.\n", ifname);
        bond_devs_unlock();
        return -1;
    }

    /* 申请新节点内存*/
    item_to_insert = (struct bond_dev_hash_item *)kzalloc(sizeof(struct bond_dev_hash_item), GFP_KERNEL);
    if (!item_to_insert)
    {
        PSN_LOG_ERR("malloc fialed.\n");
        bond_devs_unlock();
        return -1;
    }

    /* 同步bond状态信息*/
    port_desc_loc = &item_to_insert->dev_info.ports_desc;
    zte_strncpy_s((char *)item_to_insert->ifname, ifname, IFNAME_MAX_SIZE - 1);
    *port_desc_loc = *port_desc_ext;

    /* 初始化ip_hash表资源*/
    ret = bond_dev_info_init(&item_to_insert->dev_info);
    if (ret != 0)
    {
        PSN_LOG_ERR("%s bond_dev_info_init failed.\n", ifname);
        bond_devs_unlock();
        kfree(item_to_insert);
        return -1;
    }

    ret = rhashtable_insert_fast(&devs_st->bond_devs_rht, &item_to_insert->node, \
                                                          devs_st->bond_devs_rht_params);
    if (ret != 0)
    {
        PSN_LOG_ERR("insert item_to_insert: %s failed.\n", ifname);
        bond_devs_unlock();
        kfree(item_to_insert);
        return -1;
    }

    bond_devs_unlock();
    return 0;
}

/* 查询ip*/
struct remote_ip_hash_item *_look_up_ip(struct bond_dev_info *dev_info, struct ip_info *ip_pair)
{
    struct remote_ip_hash_item *item = NULL;

    if (!dev_info) {
        PSN_LOG_DEBUG("dev info is null.\n");
        return NULL;
    }

    item = rhashtable_lookup_fast(&dev_info->remote_ip_hash_list, \
                                  ip_pair,                        \
                                  dev_info->ip_hash_params);
    if (!item)
    {
        PSN_LOG_DEBUG("IP %s:%s not found\n", ip_pair->src_ip, ip_pair->ip);
        return NULL;
    }
    return item;
}

/* 根据网口名和ip查询状态机， 外部加锁*/
struct remote_ip_hash_item *look_up_ip(char *ifname, struct ip_info *ip_pair)
{
    struct bond_dev_hash_item *bond_dev = bond_dev_get_by_ifname(ifname);

    if (!bond_dev)
    {
        PSN_LOG_DEBUG("bond device: %s not found while look up ip.\n", ifname);
        return NULL;
    }

    return _look_up_ip(&bond_dev->dev_info, ip_pair);
}

static void ip_timeout_handler(struct timer_list *t)
{
    struct remote_ip_hash_item *entry = from_timer(entry, t, timeout_timer);

    PSN_LOG_ERR("Timeout occurred for IP: %s.\n", entry->ip_pair.ip);
    entry->state = STATE_ERROR;
}

int add_ip(char *ifname, struct ip_info *ip_pair, struct ip_rhash_tbl_value_st *mac_pair)
{
    int ret = 0;
    struct bond_dev_hash_item *bond_dev = bond_dev_get_by_ifname(ifname);
    struct remote_ip_hash_item *new_item;

    if (!bond_dev) {
        PSN_LOG_DEBUG("Bond device %s not found\n", ifname);
        return -ENOENT;
    }

    new_item = _look_up_ip(&bond_dev->dev_info, ip_pair);
    if (new_item != NULL)
    {
        PSN_LOG_DEBUG("dev:%s already linked remote ip: %s.\n", ifname, ip_pair->ip);
        return -1;
    }

    new_item = kzalloc(sizeof(*new_item), GFP_KERNEL);
    if (!new_item) {
        PSN_LOG_DEBUG("Failed to allocate memory for remote_ip_hash_item.\n");
        return -ENOMEM;
    }

    new_item->ip_pair = *ip_pair;
    new_item->mac_pair = *mac_pair;
    timer_setup(&new_item->timeout_timer, ip_timeout_handler, 0);
    new_item->state = STATE_NEW;

    ret = rhashtable_insert_fast(&bond_dev->dev_info.remote_ip_hash_list, \
                                 &new_item->node,                         \
                                 bond_dev->dev_info.ip_hash_params);
    if (ret) {
        PSN_LOG_DEBUG("Failed to insert IP entry.\n");
        del_timer_sync(&new_item->timeout_timer);
        kfree(new_item);
    }
    return ret;
}

int del_ip(char *ifname, struct ip_info *ip_pair)
{
    struct bond_dev_hash_item *bond_dev = bond_dev_get_by_ifname(ifname);
    struct remote_ip_hash_item *item;

    if (!bond_dev) {
        PSN_LOG_DEBUG("Bond device %s not found\n", ifname);
        return -ENOENT;
    }

    item = _look_up_ip(&bond_dev->dev_info, ip_pair);
    if (!item)
    {
        PSN_LOG_DEBUG("remote_ip_hash_item %s:%s not found\n", ip_pair->src_ip, ip_pair->ip);
        return -1;
    }

    rhashtable_remove_fast(&bond_dev->dev_info.remote_ip_hash_list, \
                           &item->node,                             \
                           bond_dev->dev_info.ip_hash_params);
    del_timer_sync(&item->timeout_timer);
    kfree(item);
    return 0;
}

/* ip hash iterate 1*/
void bond_dev_info_destroy(struct bond_dev_info *dev_info)
{
    struct rhashtable_iter iter;
    struct remote_ip_hash_item *rip_item = NULL;

    psn_mutex_lock(&dev_info->remote_ip_lock);
    rhashtable_walk_enter(&dev_info->remote_ip_hash_list, &iter);
    rhashtable_walk_start(&iter);
    while (true)
    {
        rip_item = rhashtable_walk_next(&iter);
        if (rip_item == NULL) {
            break;
        }

        if (IS_ERR(rip_item)) {
            if (PTR_ERR(rip_item) == -EAGAIN) {
                continue;
            } else {
                PSN_LOG_DEBUG("Error during rhashtable walk: %ld\n", PTR_ERR(rip_item));
                break;
            }
        }

        if (rip_item) {
            rhashtable_remove_fast(&dev_info->remote_ip_hash_list, \
                                   &rip_item->node,                \
                                   dev_info->ip_hash_params);
            PSN_LOG_DEBUG("remove ip item: %s.\n", rip_item->ip_pair.ip);
            del_timer_sync(&rip_item->timeout_timer);
            kfree(rip_item);
        }

    }
    rhashtable_walk_stop(&iter);
    rhashtable_walk_exit(&iter);
    psn_mutex_unlock(&dev_info->remote_ip_lock);
    rhashtable_destroy(&dev_info->remote_ip_hash_list);
}

void user_send_msg_normal(char *ifname, struct ip_info *ip_pair, struct ip_rhash_tbl_value_st *ip_value, uint8_t mode);
int bond_dev_notify_all_ip(char *ifname)
{
    struct bond_dev_hash_item *ifname_entry = NULL;
    struct rhashtable_iter iter;
    struct remote_ip_hash_item *rip_item = NULL;
    struct bond_dev_info *dev_info = NULL;

    ifname_entry = bond_dev_get_by_ifname(ifname);
    if (!ifname_entry)
    {
        PSN_LOG_ERR("bond dev notify all ip states failed.\n");
        return -1;
    }

    dev_info = &ifname_entry->dev_info;
    rhashtable_walk_enter(&dev_info->remote_ip_hash_list, &iter);
    rhashtable_walk_start(&iter);
    while (true) {
        rip_item = rhashtable_walk_next(&iter);
        if (rip_item == NULL)
        {
            break;
        }

        if (IS_ERR(rip_item)) {
            if (PTR_ERR(rip_item) == -EAGAIN) {
                continue;
            } else {
                PSN_LOG_DEBUG("Error during rhashtable walk: %ld\n", PTR_ERR(rip_item));
                break;
            }
        }

        user_send_msg_normal(ifname, &rip_item->ip_pair, &rip_item->mac_pair, RECV_CMD_RDMA_ADD_IP);
    }
    rhashtable_walk_stop(&iter);
    rhashtable_walk_exit(&iter);
    return 0;
}

/*****************************状态机相关操作*****************************/
int psn_msq_push(void *_send_msg);

/***********************以下为消息上送模块*****************************/

struct ker_message_node
{
    msg_entity_t msg;
    struct list_head node;
};

struct ker_msgq_st
{
    struct list_head msg_list;
    spinlock_t lock;
};
struct ker_msgq_st msg_queue = {0};

struct ker_msgq_st *psn_msg_queue_get(void)
{
    return &msg_queue;
}

int psn_msq_resource_init(void)
{
    struct ker_msgq_st *msq = psn_msg_queue_get();

    spin_lock_init(&msq->lock);
    INIT_LIST_HEAD(&msq->msg_list);
    return 0;
}

void psn_msg_resorce_destroy(void)
{
    struct ker_msgq_st *msq = psn_msg_queue_get();
    struct ker_message_node *msg_node, *tmp;

    spin_lock(&msq->lock);
    list_for_each_entry_safe(msg_node, tmp, &msq->msg_list, node)
    {
        list_del(&msg_node->node);
        kfree(msg_node);
    }
    spin_unlock(&msq->lock);
    return;
}

/* 入队---内核消息入队缓存*/
int psn_msq_push(void *_send_msg)
{
    int value = 0;
    struct ker_msgq_st *msq = psn_msg_queue_get();
    struct ker_message_node *msg_ptr = NULL;
    msg_entity_t *send_msg = (msg_entity_t *)_send_msg;

    if (!msq)
    {
        PSN_LOG_ERR("msgq null.\n");
        return -1;
    }

    value = atomic_read(&user_heart_lose);
    if (value >= HEARTBEAT_THRESHOLD)
    {
        PSN_LOG_DEBUG("psn_user is not alive.\n");
        return -1;
    }

    msg_ptr = kzalloc(sizeof(*msg_ptr), GFP_KERNEL);
    if (!msg_ptr)
    {
        PSN_LOG_ERR("malloc msg_ptr failed.\n");
        return -1;
    }
    zte_memcpy_s(&msg_ptr->msg, send_msg, sizeof(msg_entity_t));
    spin_lock(&msq->lock);
    if (is_msg_list_over_limit(&msq->msg_list, KERNEL_MSGQ_MAX_LEN))
    {
        spin_unlock(&msq->lock);
        kfree(msg_ptr);
        PSN_LOG_DEBUG(" msg list num is over max.\n");
        return -1;
    }

    list_add_tail(&msg_ptr->node, &msq->msg_list);
    spin_unlock(&msq->lock);
    return 0;
}

/* 出队---用户态主动触发消息上送*/
int psn_msq_pop(msg_entity_t *reps)
{
    struct ker_message_node *msg_ptr = NULL;
    struct ker_msgq_st *msq = psn_msg_queue_get();

    if (!msq)
    {
        PSN_LOG_ERR("msgq null wihle pop msg.\n");
        return -1;
    }

    spin_lock(&msq->lock);
    list_for_each_entry(msg_ptr, &msq->msg_list, node)
    {
        list_del(&msg_ptr->node);
        PSN_LOG_DEBUG("detected once, cmd:%u.\n", msg_ptr->msg.header.cmd);
        zte_memcpy_s(reps, &msg_ptr->msg, sizeof(msg_ptr->msg));
        kfree(msg_ptr);
        break;
    }
    spin_unlock(&msq->lock);
    return 0;
}

void user_send_msg_normal_v2(char *ifname, struct ip_info *ip_pair, struct ip_rhash_tbl_value_st *ip_value, uint8_t mode)
{
    struct bond_dev_hash_item *bond_item = NULL;
    struct bond_ports_info *ports_dec_loc = NULL;
    msg_entity_t msg = {0};

    if (NULL == ifname)
    {
        return;
    }

    bond_item = bond_dev_get_by_ifname(ifname);
    if (!bond_item)
    {
        PSN_LOG_DEBUG("Bond device %s not found\n", ifname);
        return;
    }
    ports_dec_loc = &bond_item->dev_info.ports_desc;

    msg.header.recv_state = VALID_REPS_FLAG_V2;
    msg.header.cmd = mode;
    msg.v2.slave1_port = ports_dec_loc->slave_devs[0].np_port;
    msg.v2.slave2_port = ports_dec_loc->slave_devs[1].np_port;

    msg.v2.bond1_state = ports_dec_loc->slave_devs[0].link_state;
    msg.v2.bond2_state = ports_dec_loc->slave_devs[1].link_state;

    msg.v2.slave1_is_update = ports_dec_loc->slave_devs[0].is_update;
    msg.v2.slave2_is_update = ports_dec_loc->slave_devs[1].is_update;

    msg.v2.slave1_fid = ports_dec_loc->slave_devs[0].fid;
    msg.v2.slave2_fid = ports_dec_loc->slave_devs[1].fid;

    if (ifname)
        strscpy(msg.v2.ifname, ifname, sizeof(msg.v2.ifname));

    msg.v2.ip_pair = *ip_pair;
    msg.v2.mac_pair = *ip_value;

    psn_msq_push(&msg);
}

/* 组合v4版本bond相关事件信息*/
void user_v4_bond_event(char *ifname, struct bond_dev_info *devs_info, msg_entity_t *msg)
{
    int idx = 0;
    struct psn_bond_event_msg *bond_event = &msg->v4.bond_event;
    struct bond_ports_info *ports_desc_loc = &devs_info->ports_desc; 

    zte_memcpy_s(bond_event->ifname, ifname, sizeof(bond_event->ifname));
    bond_event->slave_nums = ports_desc_loc->slaves_num;
    PSN_LOG_INFO("bond dev-%s v4 event.\n", bond_event->ifname);
    for (idx = 0; idx < ports_desc_loc->slaves_num; idx++)
    {
        msg->v4.slot_id = (ports_desc_loc->slave_devs[idx].fid & 0xffff0000) >> 16;
        bond_event->slave_stats[idx].is_update = ports_desc_loc->slave_devs[idx].is_update;
        bond_event->slave_stats[idx].link_state = ports_desc_loc->slave_devs[idx].link_state;
        bond_event->slave_stats[idx].np_port = ports_desc_loc->slave_devs[idx].np_port;
        bond_event->slave_stats[idx].is_enable = ports_desc_loc->slave_devs[idx].is_enable;
        bond_event->slave_stats[idx].fid = ports_desc_loc->slave_devs[idx].fid;
    }
    return;
}

/* 组合v4版本rdma相关信息*/
void user_v4_rdma_event(struct bond_dev_info *devs_info, msg_entity_t *msg,
                        struct ip_info *ip_pair, struct ip_rhash_tbl_value_st *ip_value)
{
    int idx = 0;
    struct bond_ports_info *ports_desc_loc = &devs_info->ports_desc;

    for (idx = 0; idx < ports_desc_loc->slaves_num; idx++)
    {
        if (ports_desc_loc->slave_devs[idx].is_enable)
        {
            msg->v4.slot_id = (ports_desc_loc->slave_devs[idx].fid & 0xffff0000) >> 16;
            break;
        }
    }

    msg->v4.rdma_event.ip_pair = *ip_pair;
    msg->v4.rdma_event.mac_pair = *ip_value;
    return;
}

void user_send_msg_normal_v4(char *ifname, struct ip_info *ip_pair, struct ip_rhash_tbl_value_st *ip_value, uint8_t mode)
{
    struct bond_dev_hash_item *bond_item = NULL;
    struct bond_dev_info *devs_info = NULL;
    msg_entity_t msg = {0};

    if (NULL == ifname)
    {
        return;
    }

    bond_item = bond_dev_get_by_ifname(ifname);
    if (!bond_item)
    {
        PSN_LOG_DEBUG("Bond device %s not found\n", ifname);
        return;
    }
    devs_info = &bond_item->dev_info;

    msg.header.recv_state = VALID_REPS_FLAG_V4;
    msg.header.cmd = mode; //
    if (mode == RECV_CMD_UPDATE_BOND ||
        mode == RECV_CMD_CREATE_BOND ||
        mode == RECV_CMD_DELETE_BOND ||
        mode == RECV_CMD_LISTENDING_NOTICE)
    {
        user_v4_bond_event(ifname, devs_info, &msg);
        psn_msq_push(&msg);
        PSN_LOG_INFO("kernel bond-%s send v4, mode:%u.\n", msg.v4.bond_event.ifname, mode);
    }
    else if(mode == RECV_CMD_RDMA_ADD_IP ||
            mode == RECV_CMD_RDMA_DEL_IP)
    {
        user_send_msg_normal_v2(ifname, ip_pair, ip_value, mode);
    }
}

void user_send_msg_normal(char *ifname, struct ip_info *ip_pair, struct ip_rhash_tbl_value_st *ip_value, uint8_t mode)
{
    struct bond_dev_hash_item *bond_item = NULL;
    struct bond_ports_info *ports_desc_loc = NULL;

    if (NULL == ifname)
    {
        return;
    }

    bond_item = bond_dev_get_by_ifname(ifname);
    if (!bond_item)
    {
        PSN_LOG_DEBUG("Bond device %s not found\n", ifname);
        return;
    }
    ports_desc_loc = &bond_item->dev_info.ports_desc;

    if (ports_desc_loc->slaves_num == SLAVE_TWO)
    {
        PSN_LOG_INFO("v2 send once.\n");
        user_send_msg_normal_v2(ifname, ip_pair, ip_value, mode);
    }
    else
    {
        PSN_LOG_INFO("v4 send once.\n");
        user_send_msg_normal_v4(ifname, ip_pair, ip_value, mode);
    }
    return;
}

void init_notify_user_stop_all(void)
{
    msg_entity_t msg = {0};
    msg.header.recv_state = VALID_REPS_FLAG_V2;
    msg.header.cmd = RECV_CMD_STOP_ALL_LISTEN;
    psn_msq_push(&msg);
}
/***********************以下为消息处理模块*****************************/
typedef int (*netlink_handler_t)(msg_entity_t *, pid_t);
typedef struct {
    int cmd;
    netlink_handler_t handler;
} netlink_cmd_handler_t;


 /* operation definition */
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
static struct nla_policy bar_genl_policy[BAR_A_MAX + 1] = {
        [BAR_A_MSG] = { .type = NLA_NUL_STRING },
};
#endif

int genl_recv_doit(struct sk_buff *skb, struct genl_info *info);
struct genl_ops bar_gnl_ops[] = {
    {
        .cmd = BAR_C_ECHO,
        .flags = 0,
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
        .policy = bar_genl_policy,
#endif
        .doit = genl_recv_doit,
        .dumpit = NULL,
    }
};

static struct genl_family bar_msg_family = {
       .hdrsize = 0,
       .name = MPF_GENRIC_NTLINK_NAME,
       .version = 1,
       .maxattr = BAR_A_MAX,
       .ops = bar_gnl_ops,
       .n_ops = 1,
};

int psn_netlink_init(void)
{
    int state=0;
    state = genl_register_family(&bar_msg_family);
    if(state)
    {
       PSN_LOG_ERR("genl_register_family error!!!\n");
       return 1;
    }

    PSN_LOG_INFO("gennetlink register success!!!\n");
    return 0;
}

void psn_netlink_remove(void)
{
    genl_unregister_family(&bar_msg_family);
    PSN_LOG_INFO("gennetlink unregister.....\n");
}

/*
* genl_msg_prepare_usr_msg : 构建netlink及gennetlink首部
* @cmd : genl_ops的cmd
* @size : gen_netlink用户数据的长度（包括用户定义的首部）
*/
static int genl_msg_prepare_usr_msg(u8 cmd, size_t size, pid_t pid, struct sk_buff **skbp)
{
    struct sk_buff *skb;
    /* create a new netlink msg */
    skb = genlmsg_new(size, GFP_KERNEL);
    if (skb == NULL)
    {
        return -ENOMEM;
    }
    /* Add a new netlink message to an skb */
    genlmsg_put(skb, pid, 0, &bar_msg_family, 0, cmd);
    *skbp = skb;
    return 0;
}

/*
* 添加用户数据，及添加一个netlink addribute
*@type : nlattr的type
*@len : nlattr中的len
*@data : 用户数据
*/
static int genl_msg_mk_usr_msg(struct sk_buff *skb, int type, void *data, int len)
{
    int rc;
    /* add a netlink attribute to a socket buffer */
    if ((rc = nla_put(skb, type, len, data)) != 0)
    {
        return rc;
    }
    return 0;
}

/**
* genl_msg_send_to_user - 通过generic netlink发送数据到netlink
*
* @data: 发送数据缓存
* @len: 数据长度 单位：byte
* @pid: 发送到的客户端pid
*/
int genl_msg_send_to_user(void *data, int len, pid_t pid)
{
    struct sk_buff *skb;
    size_t size;
    int rc;

    size = nla_total_size(len); /* total length of attribute including padding */
    rc = genl_msg_prepare_usr_msg(BAR_C_ECHO, size, pid, &skb);
    if (rc)
    {
        return rc;
    }
    rc = genl_msg_mk_usr_msg(skb, BAR_A_MSG, data, len);
    if (rc)
    {
        kfree_skb(skb);
        return rc;
    }

    rc = genlmsg_unicast(&init_net, skb, pid);
    if (rc < 0)
    {
        return rc;
    }
    return 0;
}

/******************************后端消息回调*********************************/
int bar_chan_netlink_add_bond_dev(msg_entity_t *msg, pid_t pid)
{
    msg_entity_t reps = {0};
    struct bond_ports_info bond_info = {0};
    struct ip_info ip_pair = {0};
    struct ip_rhash_tbl_value_st ip_value = {0};
    uint8_t mode = RECV_CMD_CREATE_BOND;

    PSN_LOG_INFO("create hwbond: %s.\n", msg->v2.ifname);
    msg->header.recv_state = VALID_REPS_FLAG_V2;
    bond_info.slaves_num = 2;
    bond_info.slave_devs[0].link_state = 1;
    bond_info.slave_devs[1].link_state = 1;
    bond_info.slave_devs[0].np_port = 0;
    bond_info.slave_devs[1].np_port = 4;
    bond_info.slave_devs[0].fid = msg->v2.slave1_fid;
    bond_info.slave_devs[1].fid = msg->v2.slave2_fid;
    bond_dev_insert_by_ifname(msg->v2.ifname, &bond_info);

    /* TODO: 发送消息到用户态*/
    user_send_msg_normal(msg->v2.ifname, &ip_pair, &ip_value, mode);

    genl_msg_send_to_user(&reps, sizeof(reps), pid);
    return 0;
}

int bar_chan_netlink_send_to_user(msg_entity_t *user_msg, pid_t pid)
{
    msg_entity_t reps = {0};

    psn_msq_pop(&reps);
    genl_msg_send_to_user(&reps, sizeof(reps), pid);
    return 0;
}

static void iterate_notify_all_ip(void)
{
    struct rhashtable_iter iter;
    struct bond_dev_hash_item *entry = NULL;
    struct bond_devs_rht_st *devs_st = bond_devs_rht_st_get();

    bond_devs_lock();
    rhashtable_walk_enter(&devs_st->bond_devs_rht, &iter);
    rhashtable_walk_start(&iter);
    while (true)
    {
        entry = rhashtable_walk_next(&iter);
        if (entry == NULL) {
            break;
        }

        if (IS_ERR(entry)) {
            if (PTR_ERR(entry) == -EAGAIN) {
                /* 如果是 -EAGAIN 错误，继续遍历 */
                continue;
            } else {
                /* 其他错误，停止遍历 */
                PSN_LOG_DEBUG("Error during rhashtable walk: %ld\n", PTR_ERR(entry));
                break;
            }
        }

        bond_dev_notify_all_ip(entry->ifname);
    }
    rhashtable_walk_stop(&iter);
    rhashtable_walk_exit(&iter);
    bond_devs_unlock();
}

static void iterate_notify_all_dev(void)
{
    struct rhashtable_iter iter;
    struct bond_dev_hash_item *entry = NULL;
    struct ip_info ip_pair = {0};
    struct ip_rhash_tbl_value_st ip_value = {0};
    struct bond_devs_rht_st *devs_st = bond_devs_rht_st_get();

    bond_devs_lock();
    rhashtable_walk_enter(&devs_st->bond_devs_rht, &iter);
    rhashtable_walk_start(&iter);
    while (true)
    {
        entry = rhashtable_walk_next(&iter);
        if (entry == NULL) {
            break;
        }

        if (IS_ERR(entry)) {
            if (PTR_ERR(entry) == -EAGAIN) {
                /* 如果是 -EAGAIN 错误，继续遍历 */
                continue;
            } else {
                /* 其他错误，停止遍历 */
                PSN_LOG_DEBUG("Error during rhashtable walk: %ld\n", PTR_ERR(entry));
                break;
            }
        }

        user_send_msg_normal(entry->ifname, &ip_pair, &ip_value, RECV_CMD_LISTENDING_NOTICE);
    }
    rhashtable_walk_stop(&iter);
    rhashtable_walk_exit(&iter);
    bond_devs_unlock();
}

int bar_chan_netlink_push_all_dev(msg_entity_t *user_msg, pid_t pid)
{
    msg_entity_t reps = {0};
    iterate_notify_all_dev();
    iterate_notify_all_ip();
    genl_msg_send_to_user(&reps, sizeof(reps), pid);
    return 0;
}


int bar_chan_netlink_bond_update(msg_entity_t *user_msg, pid_t pid)
{
    msg_entity_t reps = {0};
    struct bond_ports_info bond_info = {0};

    bond_info.slaves_num = SLAVE_TWO;
    bond_info.slave_devs[0].is_enable = SLAVE_ENABLE;
    bond_info.slave_devs[1].is_enable = SLAVE_ENABLE;
    bond_info.slave_devs[0].link_state = user_msg->v2.bond1_state;
    bond_info.slave_devs[1].link_state = user_msg->v2.bond2_state;

    psn_update_local_bond_state(user_msg->v2.ifname, &bond_info);
    /* 遍历ifname下的所有ip， 然后逐个执行PATH_STATE_CHANGE事件*/
    user_send_msg_normal(user_msg->v2.ifname, &user_msg->v2.ip_pair, &user_msg->v2.mac_pair, RECV_CMD_UPDATE_BOND);
    genl_msg_send_to_user(&reps, sizeof(reps), pid);
    return 0;
}

/* ip hash iterate 2*/
static void bond_dev_show_ip_link_info(struct bond_dev_info *dev_info)
{
    struct rhashtable_iter iter;
    struct remote_ip_hash_item *rip_item = NULL;
    char mac_str[18] = {0};

    rhashtable_walk_enter(&dev_info->remote_ip_hash_list, &iter);
    rhashtable_walk_start(&iter);
    while (true) {
        rip_item = rhashtable_walk_next(&iter);
        if (rip_item == NULL)
        {
            break;
        }

        if (IS_ERR(rip_item)) {
            if (PTR_ERR(rip_item) == -EAGAIN) {
                continue;
            } else {
                PSN_LOG_ERR("Error during rhashtable walk: %ld\n", PTR_ERR(rip_item));
                break;
            }
        }
        format_mac(rip_item->mac_pair.dst_mac, mac_str);
        PSN_LOG_INFO("src_ip: %s, dst_ip: %s, dst_mac: %s state: %s.\n", rip_item->ip_pair.src_ip, \
                                                                   rip_item->ip_pair.ip,       \
                                                                   mac_str,                    \
                                                                   ip_state_to_str(rip_item->state));
    }
    rhashtable_walk_stop(&iter);
    rhashtable_walk_exit(&iter);
}

static void bond_dev_show_info(char *ifname)
{
    int idx = 0;
    struct bond_dev_hash_item *ifname_entry = NULL;
    struct bond_ports_info *ports_desc_loc = NULL;

    bond_devs_lock();
    ifname_entry = bond_dev_get_by_ifname(ifname);
    if (ifname_entry != NULL)
    {
        ports_desc_loc = &ifname_entry->dev_info.ports_desc;
        PSN_LOG_INFO("dev-%s.\n", ifname_entry->ifname);
        for (idx = 0; idx < ports_desc_loc->slaves_num; idx++)
        {
            PSN_LOG_INFO("%dth slave: fid: 0x%x, link_state: %s, np_port: %u.\n", idx,
                                           ports_desc_loc->slave_devs[idx].fid,
                                           ports_desc_loc->slave_devs[idx].link_state == 1? "up" : "down",
                                           ports_desc_loc->slave_devs[idx].np_port);
        }
        bond_dev_show_ip_link_info(&ifname_entry->dev_info);
    }
    else
    {
        PSN_LOG_INFO("no dev : %s.\n", ifname);
    }
    bond_devs_unlock();
    return;
}

int bar_chan_netlink_bond_state(msg_entity_t *user_msg, pid_t pid)
{
    msg_entity_t reps = {0};

    bond_dev_show_info(user_msg->v2.ifname);
    genl_msg_send_to_user(&reps, sizeof(reps), pid);
    return 0;
}

int bar_chan_netlink_app_exit(msg_entity_t *user_msg, pid_t pid)
{
    msg_entity_t reps = {0};

    reps.header.recv_state = VALID_REPS_FLAG_V2;
    psn_msq_push(&reps);
    genl_msg_send_to_user(&reps, sizeof(reps), pid);
    return 0;
}

int bar_chan_netlink_psn_ack(msg_entity_t *user_msg, pid_t pid)
{
    msg_entity_t reps = {0};

    genl_msg_send_to_user(&reps, sizeof(reps), pid);
    return 0;
}

int bar_chan_netlink_new_ip(msg_entity_t *user_msg, pid_t pid)
{
    int ret = 0;
    msg_entity_t reps = {0};
    char bond_name[IFNAME_MAX_SIZE] = {0};

    ret = bond_devs_find_by_linked_fid(user_msg->v2.slave1_fid, bond_name);
    if (ret != 0)
    {
        PSN_LOG_ERR("not find slave_fid: 0x%x.\n", user_msg->v2.slave1_fid);
        goto out;
    }

    ret = add_ip(bond_name, &user_msg->v2.ip_pair, &user_msg->v2.mac_pair);
    user_send_msg_normal(bond_name, &user_msg->v2.ip_pair, &user_msg->v2.mac_pair, RECV_CMD_RDMA_ADD_IP);

out:
    genl_msg_send_to_user(&reps, sizeof(reps), pid);
    return 0;
}

int bar_chan_netlink_del_ip(msg_entity_t *user_msg, pid_t pid)
{
    int ret = 0;
    msg_entity_t reps = {0};
    char bond_name[IFNAME_MAX_SIZE] = {0};

    ret = bond_devs_find_by_linked_fid(user_msg->v2.slave1_fid, bond_name);
    if (ret != 0)
    {
        PSN_LOG_DEBUG("not find slave_fid: 0x%x.\n", user_msg->v2.slave1_fid);
        goto out;
    }
    del_ip(bond_name, &user_msg->v2.ip_pair);
    user_send_msg_normal(bond_name, &user_msg->v2.ip_pair, &user_msg->v2.mac_pair, RECV_CMD_RDMA_DEL_IP);
out:
    genl_msg_send_to_user(&reps, sizeof(reps), pid);
    return 0;
}

int genl_recv_doit(struct sk_buff *skb, struct genl_info *info)
{
    struct nlmsghdr *nlhdr;
    struct genlmsghdr *genlhdr;
    struct nlattr *nla;
    msg_entity_t *msg = NULL;
    uint32_t pid;
    size_t i = 0;

    static const netlink_cmd_handler_t handler_table[] = {
        {NETLINK_CMD_CREATE_NEW_BOND,   bar_chan_netlink_add_bond_dev},
        {NETLINK_CMD_USER_RECV_MSG,     bar_chan_netlink_send_to_user},
        {NETLINK_CMD_ADD_REMOTE_IP,     bar_chan_netlink_new_ip},
        {NETLINK_CMD_DEL_REMOTE_IP,     bar_chan_netlink_del_ip},
        {NETLINK_CMD_REPLY_ACK,         bar_chan_netlink_psn_ack},
        {NETLINK_CMD_APP_EXIT,          bar_chan_netlink_app_exit},
        {NETLINK_CMD_GET_BOND_STATE,    bar_chan_netlink_bond_state},
        {NETLINK_CMD_UPDATE_BOND_STATE, bar_chan_netlink_bond_update},
        {NETLINK_CMD_INIT_MSG,          bar_chan_netlink_push_all_dev},

    };

    if (skb->len < nlmsg_total_size(0)) {
        PSN_LOG_ERR("recv netlink data failed.\n");
        return -1;
    }

    nlhdr = nlmsg_hdr(skb);
    genlhdr = nlmsg_data(nlhdr);
    nla = genlmsg_data(genlhdr);
    msg = (msg_entity_t *)NLA_DATA(nla);
    pid = nlhdr->nlmsg_pid;
    atomic_set(&user_heart_lose, 0);

    for (i = 0; i < sizeof(handler_table) / sizeof(handler_table[0]); i++)
    {
        if (handler_table[i].cmd == msg->header.cmd)
        {
            handler_table[i].handler(msg, pid);
            return 0;
        }
    }
    return 0;
}

void bond_create_log(char *ifname, struct bond_ports_info *bond_info)
{
    int idx = 0;

    PSN_LOG_DEBUG("bond dev-%s created, slaves_nums: %u:\n", ifname, bond_info->slaves_num);
    for (idx = 0; idx < bond_info->slaves_num; idx++)
    {
        PSN_LOG_DEBUG("slave%u link_state: %s", idx, (bond_info->slave_devs[idx].link_state == 0? "DOWN" : "UP"));
        PSN_LOG_DEBUG("np_port: %u.\n", bond_info->slave_devs[idx].np_port);
    }
    return;
}

/****************************对外接口***************************/
int bond_dev_create_remove_event(char *ifname, struct bond_ports_info *bond_info, uint8_t mode)
{
    int ret = 0;
    uint8_t cmd = RECV_CMD_CREATE_BOND;
    struct ip_info ip_pair = {0};
    struct ip_rhash_tbl_value_st ip_value = {0};
    char ifname_key[IFNAME_MAX_SIZE] = {0};

    zte_strncpy_s(ifname_key, ifname, IFNAMSIZ - 1);
    if (mode == 1)
    {
        bond_create_log(ifname_key, bond_info);
        ret = bond_dev_insert_by_ifname(ifname_key, bond_info);
        user_send_msg_normal(ifname_key, &ip_pair, &ip_value, cmd);
        return ret;
    }
    else
    {
        PSN_LOG_DEBUG("delete bond %s.\n", ifname_key);
        user_send_msg_normal(ifname_key, &ip_pair, &ip_value, RECV_CMD_DELETE_BOND);
        ret = bond_dev_del_by_ifname(ifname_key);
        return ret;
    }
}

int bond_dev_update_event(char *ifname, struct bond_ports_info *bond_info)
{
    msg_entity_t reps = {0};
    char ifname_key[IFNAME_MAX_SIZE] = {0};

    zte_strncpy_s(ifname_key, ifname, IFNAMSIZ - 1);
    psn_update_local_bond_state(ifname_key, bond_info);
    user_send_msg_normal(ifname_key, &reps.v2.ip_pair, &reps.v2.mac_pair, RECV_CMD_UPDATE_BOND);
    return 0;
}

struct zxdh_rdma_to_eth_ip_para {
    char *ifname;
    u32 src_ip[4];
    u32 dst_ip[4];
    u64 src_mac;
    u64 dst_mac;
    u32 linked_fid;
    u8 ipv4 : 1;
    u8 mode : 1;
};

void rdma_add_del_ip(struct zxdh_rdma_to_eth_ip_para *info)
{
    int ret = 0;
    struct ip_info ip_pair = {0};
    struct ip_rhash_tbl_value_st mac_pair = {0};
    char bond_name[32] = {0};

    /* TODO将传递下来的ifname转化为bond口的名字*/
    rdma_mac_fomat_convert(info->src_mac, mac_pair.src_mac);
    rdma_mac_fomat_convert(info->dst_mac, mac_pair.dst_mac);
    mac_pair.type = (uint32_t)(!info->ipv4);
    mac_pair.src_mac_rsv[0] = (uint32_t)(!info->ipv4);

    ip_to_string(info->src_ip, ip_pair.src_ip, sizeof(ip_pair.src_ip), info->ipv4);
    ip_to_string(info->dst_ip, ip_pair.ip, sizeof(ip_pair.ip), info->ipv4);

    PSN_LOG_DEBUG("rdma socket link: rdma_add_del_ip called.\n");
    ret = bond_devs_find_by_linked_fid(info->linked_fid, bond_name);
    if (ret != 0)
    {
        PSN_LOG_ERR("do not find link name.\n");
        return;
    }

    if (info->mode == 1)
    {
        PSN_LOG_DEBUG("add_item, %s-->%s, mode:%u.\n", ip_pair.src_ip, ip_pair.ip, info->mode);
        add_ip(bond_name, &ip_pair, &mac_pair);
        user_send_msg_normal(bond_name, &ip_pair, &mac_pair, RECV_CMD_RDMA_ADD_IP);
    }
    else
    {
        PSN_LOG_DEBUG("del_item, %s-->%s, mode:%u.\n", ip_pair.src_ip, ip_pair.ip, info->mode);
        del_ip(bond_name, &ip_pair);
        user_send_msg_normal(bond_name, &ip_pair, &mac_pair, RECV_CMD_RDMA_DEL_IP);
    }
    return;
}
EXPORT_SYMBOL(rdma_add_del_ip);

/********************初始化函数和退出函数************************/
int psn_init(void)
{
    int ret = 0;
    ret = bond_devs_rht_init();
    if (ret != 0)
    {
        PSN_LOG_ERR("bond_devs_rht_init failed.\n");
        return -1;
    }

    ret = psn_msq_resource_init();
    if (ret != 0)
    {
        PSN_LOG_ERR("psn_msq_resource_init failed.\n");
        bond_devs_rht_destory();
        return -1;
    }

    PSN_LOG_INFO("psn module loaded!.\n");
    psn_netlink_init();

    timer_setup(&heartbeat_timer, heartbeat_timer_callback, 0);
    mod_timer(&heartbeat_timer, jiffies + msecs_to_jiffies(1));

    /* 通知停止监听*/
    init_notify_user_stop_all();
    return 0;
}

void psn_exit(void)
{
    init_notify_user_stop_all();
    del_timer_sync(&heartbeat_timer);
    psn_netlink_remove();
    psn_msg_resorce_destroy();
    bond_devs_rht_destory();
    return;
}