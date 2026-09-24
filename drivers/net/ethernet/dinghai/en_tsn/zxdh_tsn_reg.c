#include <linux/dinghai/log.h>
#include "zxdh_tsn.h"
#include "zxdh_tsn_reg.h"
#include "zxdh_tsn_comm.h"

int32_t tsn_read(uint64_t base_addr, uint32_t offset, uint32_t* p_val)
{
    if (IS_ERR_OR_NULL((void*)(base_addr)))
    {
        DH_LOG_ERR(MODULE_TSN, "base_addr 0x%llx invalid.\n", base_addr);
        return -EINVAL;
    }

    *p_val = readl((const volatile void *)(base_addr + offset));

    return TSN_OK;
}

int32_t tsn_write(uint64_t base_addr, uint32_t offset, uint32_t val)
{
    if (IS_ERR_OR_NULL((void*)(base_addr)))
    {
        DH_LOG_ERR(MODULE_TSN, "base_addr 0x%llx invalid.\n", base_addr);
        return -EINVAL;
    }

    writel(val, (volatile void *)(base_addr + offset));

    return TSN_OK;
}

int32_t tsn_reg_read(struct zxdh_tsn_private* tsn, uint32_t offset, uint32_t* p_val)
{
    int32_t ret = 0;

    ZXDH_TSN_COMM_CHECK_POINT(tsn);
    ZXDH_TSN_COMM_CHECK_POINT(p_val);

    ret = tsn_read(tsn->tsn_reg_base_addr, offset, p_val);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_reg_write(struct zxdh_tsn_private* tsn, uint32_t offset, uint32_t val)
{
    int32_t ret = 0;

    ZXDH_TSN_COMM_CHECK_POINT(tsn);

    ret = tsn_write(tsn->tsn_reg_base_addr, offset, val);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_enable_set(struct zxdh_tsn_private* tsn, uint32_t enable)
{
    int32_t ret = 0;

    ret = tsn_reg_write(tsn, TSN_PORT_QBV_ENABLE, enable);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_enable_get(struct zxdh_tsn_private* tsn, uint32_t* p_enable)
{
    int32_t ret = 0;

    ret = tsn_reg_read(tsn, TSN_PORT_QBV_ENABLE, p_enable);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_phy_port_set(struct zxdh_tsn_private* tsn, uint32_t phy_port)
{
    int32_t ret = 0;

    ret = tsn_reg_write(tsn, TSN_PORT_PHY_PORT_SEL, phy_port);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_phy_port_get(struct zxdh_tsn_private* tsn, uint32_t* p_phy_port)
{
    int32_t ret = 0;

    ret = tsn_reg_read(tsn, TSN_PORT_PHY_PORT_SEL, p_phy_port);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_timer_id_set(struct zxdh_tsn_private* tsn, uint32_t timer_id)
{
    int32_t ret = 0;

    ret = tsn_reg_write(tsn, TSN_PORT_TIME_SEL, timer_id);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_timer_id_get(struct zxdh_tsn_private* tsn, uint32_t* p_time_id)
{
    int32_t ret = 0;

    ret = tsn_reg_read(tsn, TSN_PORT_TIME_SEL, p_time_id);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_status_get(struct zxdh_tsn_private* tsn, uint32_t* p_ram_n, uint32_t* p_status)
{
    int32_t  ret = 0;
    uint32_t val = 0;

    ZXDH_TSN_COMM_CHECK_POINT(p_ram_n);
    ZXDH_TSN_COMM_CHECK_POINT(p_status);

    ret = tsn_reg_read(tsn, TSN_PORT_READ_RAM_N, &val);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    *p_ram_n = val & 0x3;
    *p_status = (val & 0x3C) >> 2;

    return TSN_OK;
}

int32_t tsn_port_base_time_l_set(struct zxdh_tsn_private* tsn, uint32_t base_time)
{
    int32_t ret = 0;

    ret = tsn_reg_write(tsn, TSN_PORT_BASE_TIME_L, base_time);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_base_time_h_set(struct zxdh_tsn_private* tsn, uint32_t base_time)
{
    int32_t ret = 0;

    ret = tsn_reg_write(tsn, TSN_PORT_BASE_TIME_H, base_time);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_base_time_set(struct zxdh_tsn_private* tsn, uint64_t base_time)
{
    int32_t  ret = 0;
    uint32_t base_time_l = (uint32_t)((base_time) & 0xffffffff);
    uint32_t base_time_h = (uint32_t)((base_time >> 32) & 0xffffffff);

    ret = tsn_port_base_time_l_set(tsn, base_time_l);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    ret = tsn_port_base_time_h_set(tsn, base_time_h);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_base_time_get(struct zxdh_tsn_private* tsn, uint64_t* p_base_time)
{
    int32_t  ret = 0;
    uint32_t base_time_l = 0;
    uint32_t base_time_h = 0;

    ZXDH_TSN_COMM_CHECK_POINT(p_base_time);

    ret = tsn_reg_read(tsn, TSN_PORT_BASE_TIME_L, &base_time_l);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    ret = tsn_reg_read(tsn, TSN_PORT_BASE_TIME_H, &base_time_h);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    *p_base_time = (uint64_t)((((uint64_t)(base_time_h)) << 32) | ((uint64_t)(base_time_l)));

    return TSN_OK;
}

int32_t tsn_port_cycle_time_l_set(struct zxdh_tsn_private* tsn, uint32_t cycle_time)
{
    int32_t ret = 0;

    ret = tsn_reg_write(tsn, TSN_PORT_CYCLE_TIME_L, cycle_time);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_cycle_time_h_set(struct zxdh_tsn_private* tsn, uint32_t cycle_time)
{
    int32_t ret = 0;

    ret = tsn_reg_write(tsn, TSN_PORT_CYCLE_TIME_H, cycle_time);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_cycle_time_set(struct zxdh_tsn_private* tsn, uint64_t cycle_time)
{
    int32_t  ret = 0;
    uint32_t cycle_time_l = (uint32_t)((cycle_time) & 0x000fffff);
    uint32_t cycle_time_h = (uint32_t)((cycle_time >> 20) & 0x000fffff);

    ret = tsn_port_cycle_time_l_set(tsn, cycle_time_l);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    ret = tsn_port_cycle_time_h_set(tsn, cycle_time_h);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_cycle_time_get(struct zxdh_tsn_private* tsn, uint64_t* p_cycle_time)
{
    int32_t  ret = 0;
    uint32_t cycle_time_l = 0;
    uint32_t cycle_time_h = 0;

    ZXDH_TSN_COMM_CHECK_POINT(p_cycle_time);

    ret = tsn_reg_read(tsn, TSN_PORT_CYCLE_TIME_L, &cycle_time_l);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    ret = tsn_reg_read(tsn, TSN_PORT_CYCLE_TIME_H, &cycle_time_h);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    *p_cycle_time = (uint64_t)((((uint64_t)(cycle_time_h)) << 20) | ((uint64_t)(cycle_time_l)));

    return TSN_OK;
}

int32_t tsn_port_guard_band_time_set(struct zxdh_tsn_private* tsn, uint32_t cos, uint32_t band_time)
{
    int32_t ret = 0;

    ZXDH_TSN_COMM_CHECK_INDEX_MAX(cos, TSN_PORT_QUEUE_MAX);

    ret = tsn_reg_write(tsn, TSN_PORT_GUARD_BAND_TIME + (cos * 4), band_time);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_guard_band_time_get(struct zxdh_tsn_private* tsn, uint32_t cos, uint32_t* p_band_time)
{
    int32_t ret = 0;

    ZXDH_TSN_COMM_CHECK_INDEX_MAX(cos, TSN_PORT_QUEUE_MAX);

    ret = tsn_reg_read(tsn, TSN_PORT_GUARD_BAND_TIME + (cos * 4), p_band_time);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_default_gate_set(struct zxdh_tsn_private* tsn, uint32_t gate_state)
{
    int32_t ret = 0;

    ret = tsn_reg_write(tsn, TSN_PORT_DEFAULT_GATE_EN, gate_state);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_default_gate_get(struct zxdh_tsn_private* tsn, uint32_t* p_gate_state)
{
    int32_t ret = 0;

    ret = tsn_reg_read(tsn, TSN_PORT_DEFAULT_GATE_EN, p_gate_state);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_change_gate_set(struct zxdh_tsn_private* tsn, uint32_t gate_state)
{
    int32_t ret = 0;

    ret = tsn_reg_write(tsn, TSN_PORT_CHANGE_GATE_EN, gate_state);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_init_finish_set(struct zxdh_tsn_private* tsn, uint32_t init_finish)
{
    int32_t ret = 0;

    ret = tsn_reg_write(tsn, TSN_PORT_INIT_FINISH, init_finish);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_init_finish_get(struct zxdh_tsn_private* tsn, uint32_t* p_init_finish)
{
    int32_t ret = 0;

    ret = tsn_reg_read(tsn, TSN_PORT_INIT_FINISH, p_init_finish);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_change_en_set(struct zxdh_tsn_private* tsn, uint32_t change_en)
{
    int32_t ret = 0;

    ret = tsn_reg_write(tsn, TSN_PORT_CHANGE_EN, change_en);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_change_en_get(struct zxdh_tsn_private* tsn, uint32_t* p_change_en)
{
    int32_t ret = 0;

    ret = tsn_reg_read(tsn, TSN_PORT_CHANGE_EN, p_change_en);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_gcl_num_set(struct zxdh_tsn_private* tsn, uint32_t ram_n, uint32_t gcl_num)
{
    int32_t  ret = 0;
    uint32_t tsn_port_gcl_num[TSN_PORT_RAM_NUM] = {TSN_PORT_GCL_NUM0, TSN_PORT_GCL_NUM1};

    ZXDH_TSN_COMM_CHECK_INDEX_MAX(ram_n, TSN_PORT_RAM_MAX);

    ret = tsn_reg_write(tsn, tsn_port_gcl_num[ram_n], gcl_num);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_gcl_num_get(struct zxdh_tsn_private* tsn, uint32_t ram_n, uint32_t* p_gcl_num)
{
    int32_t  ret = 0;
    uint32_t tsn_port_gcl_num[TSN_PORT_RAM_NUM] = {TSN_PORT_GCL_NUM0, TSN_PORT_GCL_NUM1};

    ZXDH_TSN_COMM_CHECK_INDEX_MAX(ram_n, TSN_PORT_RAM_MAX);

    ret = tsn_reg_read(tsn, tsn_port_gcl_num[ram_n], p_gcl_num);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_gcl_control_set(struct zxdh_tsn_private* tsn, uint32_t ram_n, uint32_t index, uint32_t gate_state, uint32_t internal)
{
    int32_t  ret = 0;
    uint32_t tsn_port_gcl_value[TSN_PORT_RAM_NUM] = {TSN_PORT_GCL_VALUE0, TSN_PORT_GCL_VALUE1};

    ZXDH_TSN_COMM_CHECK_INDEX_MAX(ram_n, TSN_PORT_RAM_MAX);
    ZXDH_TSN_COMM_CHECK_INDEX_MAX(index, TSN_PORT_GCL_MAX);

    ret = tsn_reg_write(tsn, tsn_port_gcl_value[ram_n] + (index * 4), (gate_state << 24) | internal);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_disable_set(struct zxdh_tsn_private* tsn)
{
    int32_t ret = 0;

    ret = tsn_port_enable_set(tsn, TSN_PORT_GATE_DISABLE);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    ret = tsn_port_init_finish_set(tsn, TSN_PORT_INIT_DISABLE);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    ret = tsn_port_change_en_set(tsn, TSN_PORT_CHANGE_DISABLE);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    return TSN_OK;
}

int32_t tsn_port_real_tod_time_get(struct zxdh_tsn_private* tsn, uint64_t* p_tod_time)
{
    int32_t  ret = 0;
    uint32_t tsn_timer_id = 0;
    uint32_t tod_second_h = 0;
    uint32_t tod_second_l = 0;
    uint32_t tod_nanosecond = 0;

    uint32_t tsn_real_tod_nanosecond_reg_offset[TSN_PORT_TIMER_ID_NUM] = {
        TSN0_REAL_TOD_NANOSECOND, TSN1_REAL_TOD_NANOSECOND, TSN2_REAL_TOD_NANOSECOND, TSN3_REAL_TOD_NANOSECOND
    };
    uint32_t tsn_real_high_tod_second_reg_offset[TSN_PORT_TIMER_ID_NUM] = {
        TSN0_REAL_HIGH_TOD_SECOND, TSN1_REAL_HIGH_TOD_SECOND, TSN2_REAL_HIGH_TOD_SECOND, TSN3_REAL_HIGH_TOD_SECOND
    };
    uint32_t tsn_real_lower_tod_second_reg_offset[TSN_PORT_TIMER_ID_NUM] = {
        TSN0_REAL_LOWER_TOD_SECOND, TSN1_REAL_LOWER_TOD_SECOND, TSN2_REAL_LOWER_TOD_SECOND, TSN3_REAL_LOWER_TOD_SECOND
    };

    ZXDH_TSN_COMM_CHECK_POINT(p_tod_time);

    ret = tsn_port_timer_id_get(tsn, &tsn_timer_id);
    ZXDH_TSN_COMM_CHECK_RC(ret);
    ZXDH_TSN_COMM_CHECK_INDEX_MAX(tsn_timer_id, TSN_PORT_TIMER_ID_MAX);

    ret = tsn_read(tsn->pci_ioremap_addr + 0xC000, tsn_real_high_tod_second_reg_offset[tsn_timer_id], &tod_second_h);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    ret = tsn_read(tsn->pci_ioremap_addr + 0xC000, tsn_real_lower_tod_second_reg_offset[tsn_timer_id], &tod_second_l);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    ret = tsn_read(tsn->pci_ioremap_addr + 0xC000, tsn_real_tod_nanosecond_reg_offset[tsn_timer_id], &tod_nanosecond);
    ZXDH_TSN_COMM_CHECK_RC(ret);

    *p_tod_time = ((((uint64_t)tod_second_h << 32) | (uint64_t)tod_second_l) * NSEC_PER_SEC) + tod_nanosecond;

    return TSN_OK;
}
