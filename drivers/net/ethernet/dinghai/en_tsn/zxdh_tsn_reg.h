#ifndef __ZXDH_TSN_REG_H__
#define __ZXDH_TSN_REG_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <linux/types.h>
#include <linux/dinghai/driver.h>

#define TSN_PORT_REG_BAR_SIZE               (0x4000)
#define TSN_PORT_REG_BAR_OFFSET             (0x14000)

#define TSN_PORT_QBV_ENABLE                 (0x0004)
#define TSN_PORT_PHY_PORT_SEL               (0x0008)
#define TSN_PORT_TIME_SEL                   (0x000C)
#define TSN_PORT_CLK_FREQ                   (0x0014)
#define TSN_PORT_READ_RAM_N                 (0x0018)
#define TSN_PORT_EXE_TIME                   (0x001C)
#define TSN_PORT_ITR_SHIFT                  (0x0020)
#define TSN_PORT_BASE_TIME_H                (0x0024)
#define TSN_PORT_BASE_TIME_L                (0x0028)
#define TSN_PORT_CYCLE_TIME_H               (0x0030)
#define TSN_PORT_CYCLE_TIME_L               (0x0034)
#define TSN_PORT_GUARD_BAND_TIME            (0x0040)
#define TSN_PORT_DEFAULT_GATE_EN            (0x0060)
#define TSN_PORT_CHANGE_GATE_EN             (0x0064)
#define TSN_PORT_INIT_FINISH                (0x0068)
#define TSN_PORT_CHANGE_EN                  (0x006C)
#define TSN_PORT_GCL_NUM0                   (0x0070)
#define TSN_PORT_GCL_NUM1                   (0x0074)
#define TSN_PORT_GCL_VALUE0                 (0x1000)
#define TSN_PORT_GCL_VALUE1                 (0x2000)

#define TSN0_REAL_TOD_NANOSECOND            (0x4240)
#define TSN0_REAL_LOWER_TOD_SECOND          (0x4244)
#define TSN0_REAL_HIGH_TOD_SECOND           (0x4248)
#define TSN1_REAL_TOD_NANOSECOND            (0x424C)
#define TSN1_REAL_LOWER_TOD_SECOND          (0x4250)
#define TSN1_REAL_HIGH_TOD_SECOND           (0x4254)
#define TSN2_REAL_TOD_NANOSECOND            (0x4258)
#define TSN2_REAL_LOWER_TOD_SECOND          (0x425C)
#define TSN2_REAL_HIGH_TOD_SECOND           (0x4260)
#define TSN3_REAL_TOD_NANOSECOND            (0x4264)
#define TSN3_REAL_LOWER_TOD_SECOND          (0x4268)
#define TSN3_REAL_HIGH_TOD_SECOND           (0x426C)

int32_t tsn_read(uint64_t base_addr, uint32_t offset, uint32_t* p_val);
int32_t tsn_write(uint64_t base_addr, uint32_t offset, uint32_t val);
int32_t tsn_reg_read(struct zxdh_tsn_private* tsn, uint32_t offset, uint32_t* p_val);
int32_t tsn_reg_write(struct zxdh_tsn_private* tsn, uint32_t offset, uint32_t val);
int32_t tsn_port_enable_set(struct zxdh_tsn_private* tsn, uint32_t enable);
int32_t tsn_port_enable_get(struct zxdh_tsn_private* tsn, uint32_t* p_enable);
int32_t tsn_port_phy_port_set(struct zxdh_tsn_private* tsn, uint32_t phy_port);
int32_t tsn_port_phy_port_get(struct zxdh_tsn_private* tsn, uint32_t* p_phy_port);
int32_t tsn_port_timer_id_set(struct zxdh_tsn_private* tsn, uint32_t timer_id);
int32_t tsn_port_timer_id_get(struct zxdh_tsn_private* tsn, uint32_t* p_time_id);
int32_t tsn_port_status_get(struct zxdh_tsn_private* tsn, uint32_t* p_ram_n, uint32_t* p_status);
int32_t tsn_port_base_time_l_set(struct zxdh_tsn_private* tsn, uint32_t base_time);
int32_t tsn_port_base_time_h_set(struct zxdh_tsn_private* tsn, uint32_t base_time);
int32_t tsn_port_base_time_set(struct zxdh_tsn_private* tsn, uint64_t base_time);
int32_t tsn_port_base_time_get(struct zxdh_tsn_private* tsn, uint64_t* p_base_time);
int32_t tsn_port_cycle_time_l_set(struct zxdh_tsn_private* tsn, uint32_t cycle_time);
int32_t tsn_port_cycle_time_h_set(struct zxdh_tsn_private* tsn, uint32_t cycle_time);
int32_t tsn_port_cycle_time_set(struct zxdh_tsn_private* tsn, uint64_t cycle_time);
int32_t tsn_port_cycle_time_get(struct zxdh_tsn_private* tsn, uint64_t* p_cycle_time);
int32_t tsn_port_guard_band_time_set(struct zxdh_tsn_private* tsn, uint32_t cos, uint32_t band_time);
int32_t tsn_port_guard_band_time_get(struct zxdh_tsn_private* tsn, uint32_t cos, uint32_t* p_band_time);
int32_t tsn_port_default_gate_set(struct zxdh_tsn_private* tsn, uint32_t gate_state);
int32_t tsn_port_default_gate_get(struct zxdh_tsn_private* tsn, uint32_t* p_gate_state);
int32_t tsn_port_change_gate_set(struct zxdh_tsn_private* tsn, uint32_t gate_state);
int32_t tsn_port_init_finish_set(struct zxdh_tsn_private* tsn, uint32_t init_finish);
int32_t tsn_port_init_finish_get(struct zxdh_tsn_private* tsn, uint32_t* p_init_finish);
int32_t tsn_port_change_en_set(struct zxdh_tsn_private* tsn, uint32_t change_en);
int32_t tsn_port_change_en_get(struct zxdh_tsn_private* tsn, uint32_t* p_change_en);
int32_t tsn_port_gcl_num_set(struct zxdh_tsn_private* tsn, uint32_t ram_n, uint32_t gcl_num);
int32_t tsn_port_gcl_num_get(struct zxdh_tsn_private* tsn, uint32_t ram_n, uint32_t* p_gcl_num);
int32_t tsn_port_gcl_control_set(struct zxdh_tsn_private* tsn, uint32_t ram_n, uint32_t index, uint32_t gate_state, uint32_t internal);
int32_t tsn_port_disable_set(struct zxdh_tsn_private* tsn);
int32_t tsn_port_real_tod_time_get(struct zxdh_tsn_private* tsn, uint64_t* p_tod_time);

#ifdef __cplusplus
}
#endif

#endif /* __ZXDH_TSN_REG_H__ */
