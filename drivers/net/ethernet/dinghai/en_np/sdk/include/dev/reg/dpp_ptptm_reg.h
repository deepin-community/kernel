
#ifndef _DPP_PTPTM_REG_H_
#define _DPP_PTPTM_REG_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct dpp_ptptm_ptp_top_pp1s_interrupt_t
{
    ZXIC_UINT32 int_state;
    ZXIC_UINT32 int_test;
    ZXIC_UINT32 int_clr;
    ZXIC_UINT32 int_en;
}DPP_PTPTM_PTP_TOP_PP1S_INTERRUPT_T;

typedef struct dpp_ptptm_ptp_top_pp1s_external_select_t
{
    ZXIC_UINT32 pp1s_external_select;
}DPP_PTPTM_PTP_TOP_PP1S_EXTERNAL_SELECT_T;

typedef struct dpp_ptptm_ptp_top_pp1s_out_select_t
{
    ZXIC_UINT32 pp1s_out_sel;
}DPP_PTPTM_PTP_TOP_PP1S_OUT_SELECT_T;

typedef struct dpp_ptptm_ptp_top_test_pp1s_select_t
{
    ZXIC_UINT32 test_pp1s_sel;
}DPP_PTPTM_PTP_TOP_TEST_PP1S_SELECT_T;

typedef struct dpp_ptptm_ptp_top_local_pp1s_en_t
{
    ZXIC_UINT32 local_pp1s_en;
}DPP_PTPTM_PTP_TOP_LOCAL_PP1S_EN_T;

typedef struct dpp_ptptm_ptp_top_local_pp1s_adjust_t
{
    ZXIC_UINT32 local_pp1s_adjust_sel;
    ZXIC_UINT32 local_pp1s_adjust_en;
}DPP_PTPTM_PTP_TOP_LOCAL_PP1S_ADJUST_T;

typedef struct dpp_ptptm_ptp_top_local_pp1s_adjust_value_t
{
    ZXIC_UINT32 local_pp1s_adjust_value;
}DPP_PTPTM_PTP_TOP_LOCAL_PP1S_ADJUST_VALUE_T;

typedef struct dpp_ptptm_ptp_top_pp1s_to_np_select_t
{
    ZXIC_UINT32 pp1s_to_np_sel;
}DPP_PTPTM_PTP_TOP_PP1S_TO_NP_SELECT_T;

typedef struct dpp_ptptm_ptp_top_pd_u1_sel_t
{
    ZXIC_UINT32 pd_u1_sel1;
    ZXIC_UINT32 pd_u1_sel0;
}DPP_PTPTM_PTP_TOP_PD_U1_SEL_T;

typedef struct dpp_ptptm_ptp_top_pd_u1_pd0_shift_t
{
    ZXIC_UINT32 pd_u1_pd0_shift;
}DPP_PTPTM_PTP_TOP_PD_U1_PD0_SHIFT_T;

typedef struct dpp_ptptm_ptp_top_pd_u1_pd1_shift_t
{
    ZXIC_UINT32 pd_u1_pd1_shift;
}DPP_PTPTM_PTP_TOP_PD_U1_PD1_SHIFT_T;

typedef struct dpp_ptptm_ptp_top_pd_u1_result_t
{
    ZXIC_UINT32 pd_u1_result_sign;
    ZXIC_UINT32 pd_u1_overflow;
    ZXIC_UINT32 pd_u1_result;
}DPP_PTPTM_PTP_TOP_PD_U1_RESULT_T;

typedef struct dpp_ptptm_ptp_top_pd_u2_sel_t
{
    ZXIC_UINT32 pd_u2_sel1;
    ZXIC_UINT32 pd_u2_sel0;
}DPP_PTPTM_PTP_TOP_PD_U2_SEL_T;

typedef struct dpp_ptptm_ptp_top_pd_u2_pd0_shift_t
{
    ZXIC_UINT32 pd_u2_pd0_shift;
}DPP_PTPTM_PTP_TOP_PD_U2_PD0_SHIFT_T;

typedef struct dpp_ptptm_ptp_top_pd_u2_pd1_shift_t
{
    ZXIC_UINT32 pd_u2_pd1_shift;
}DPP_PTPTM_PTP_TOP_PD_U2_PD1_SHIFT_T;

typedef struct dpp_ptptm_ptp_top_pd_u2_result_t
{
    ZXIC_UINT32 pd_u2_result_sign;
    ZXIC_UINT32 pd_u2_overflow;
    ZXIC_UINT32 pd_u2_result;
}DPP_PTPTM_PTP_TOP_PD_U2_RESULT_T;

typedef struct dpp_ptptm_ptp_top_tsn_group_nanosecond_delay0_t
{
    ZXIC_UINT32 tsn_group_nanosecond_delay0;
}DPP_PTPTM_PTP_TOP_TSN_GROUP_NANOSECOND_DELAY0_T;

typedef struct dpp_ptptm_ptp_top_tsn_group_fracnanosecond_delay0_t
{
    ZXIC_UINT32 tsn_group_fracnanosecond_delay0;
}DPP_PTPTM_PTP_TOP_TSN_GROUP_FRACNANOSECOND_DELAY0_T;

typedef struct dpp_ptptm_ptp_top_tsn_group_nanosecond_delay1_t
{
    ZXIC_UINT32 tsn_group_nanosecond_delay1;
}DPP_PTPTM_PTP_TOP_TSN_GROUP_NANOSECOND_DELAY1_T;

typedef struct dpp_ptptm_ptp_top_tsn_group_fracnanosecond_delay1_t
{
    ZXIC_UINT32 tsn_group_fracnanosecond_delay1;
}DPP_PTPTM_PTP_TOP_TSN_GROUP_FRACNANOSECOND_DELAY1_T;

typedef struct dpp_ptptm_ptp_top_tsn_group_nanosecond_delay2_t
{
    ZXIC_UINT32 tsn_group_nanosecond_delay2;
}DPP_PTPTM_PTP_TOP_TSN_GROUP_NANOSECOND_DELAY2_T;

typedef struct dpp_ptptm_ptp_top_tsn_group_fracnanosecond_delay2_t
{
    ZXIC_UINT32 tsn_group_fracnanosecond_delay2;
}DPP_PTPTM_PTP_TOP_TSN_GROUP_FRACNANOSECOND_DELAY2_T;

typedef struct dpp_ptptm_ptp_top_tsn_group_nanosecond_delay3_t
{
    ZXIC_UINT32 tsn_group_nanosecond_delay3;
}DPP_PTPTM_PTP_TOP_TSN_GROUP_NANOSECOND_DELAY3_T;

typedef struct dpp_ptptm_ptp_top_tsn_group_fracnanosecond_delay3_t
{
    ZXIC_UINT32 tsn_group_fracnanosecond_delay3;
}DPP_PTPTM_PTP_TOP_TSN_GROUP_FRACNANOSECOND_DELAY3_T;

typedef struct dpp_ptptm_ptp_top_tsn_ptp1588_rdma_nanosecond_delay_t
{
    ZXIC_UINT32 ptp1588_rdma_nanosecond_delay;
}DPP_PTPTM_PTP_TOP_TSN_PTP1588_RDMA_NANOSECOND_DELAY_T;

typedef struct dpp_ptptm_ptp_top_ptp1588_rdma_fracnanosecond_delay_t
{
    ZXIC_UINT32 ptp1588_rdma_fracnanosecond_delay;
}DPP_PTPTM_PTP_TOP_PTP1588_RDMA_FRACNANOSECOND_DELAY_T;

typedef struct dpp_ptptm_ptp_top_ptp1588_np_nanosecond_delay_t
{
    ZXIC_UINT32 ptp1588_np_nanosecond_delay;
}DPP_PTPTM_PTP_TOP_PTP1588_NP_NANOSECOND_DELAY_T;

typedef struct dpp_ptptm_ptp_top_ptp1588_np_fracnanosecond_delay_t
{
    ZXIC_UINT32 ptp1588_np_fracnanosecond_delay;
}DPP_PTPTM_PTP_TOP_PTP1588_NP_FRACNANOSECOND_DELAY_T;

typedef struct dpp_ptptm_ptp_top_time_sync_period_t
{
    ZXIC_UINT32 time_sync_period;
}DPP_PTPTM_PTP_TOP_TIME_SYNC_PERIOD_T;

typedef struct dpp_ptptm_ptptm_module_id_t
{
    ZXIC_UINT32 module_id;
}DPP_PTPTM_PTPTM_MODULE_ID_T;

typedef struct dpp_ptptm_ptptm_module_version_t
{
    ZXIC_UINT32 module_major_version;
    ZXIC_UINT32 module_minor_version;
}DPP_PTPTM_PTPTM_MODULE_VERSION_T;

typedef struct dpp_ptptm_ptptm_module_date_t
{
    ZXIC_UINT32 year;
    ZXIC_UINT32 month;
    ZXIC_UINT32 date;
}DPP_PTPTM_PTPTM_MODULE_DATE_T;

typedef struct dpp_ptptm_ptptm_interrupt_status_t
{
    ZXIC_UINT32 pps_in_status;
    ZXIC_UINT32 fifo_almost_full_status;
    ZXIC_UINT32 fifo_no_empty_status;
    ZXIC_UINT32 trigger_output_status;
    ZXIC_UINT32 trigger_input_status;
}DPP_PTPTM_PTPTM_INTERRUPT_STATUS_T;

typedef struct dpp_ptptm_ptptm_interrupt_event_t
{
    ZXIC_UINT32 pps_in_event;
    ZXIC_UINT32 fifo_almost_full_event;
    ZXIC_UINT32 fifo_no_empty_event;
    ZXIC_UINT32 trigger_output_event;
    ZXIC_UINT32 trigger_input_event;
}DPP_PTPTM_PTPTM_INTERRUPT_EVENT_T;

typedef struct dpp_ptptm_ptptm_interrupt_mask_t
{
    ZXIC_UINT32 pps_in_event_mask;
    ZXIC_UINT32 fifo_almost_full_event_mask;
    ZXIC_UINT32 fifo_no_empty_event_mask;
    ZXIC_UINT32 trigger_output_event_mask;
    ZXIC_UINT32 trigger_input_eventt_mask;
}DPP_PTPTM_PTPTM_INTERRUPT_MASK_T;

typedef struct dpp_ptptm_ptptm_interrupt_test_t
{
    ZXIC_UINT32 trigger_pps_in_event_test;
    ZXIC_UINT32 trigger_fifo_almost_full_event_test;
    ZXIC_UINT32 trigger_fifo_no_empty_event_test;
    ZXIC_UINT32 trigger_output_event_test;
    ZXIC_UINT32 trigger_input_event_test;
}DPP_PTPTM_PTPTM_INTERRUPT_TEST_T;

typedef struct dpp_ptptm_ptptm_hw_clock_cycle_integer_t
{
    ZXIC_UINT32 integeral_nanosecond_of_hw_clock_cycle;
}DPP_PTPTM_PTPTM_HW_CLOCK_CYCLE_INTEGER_T;

typedef struct dpp_ptptm_ptptm_hw_clock_cycle_fraction_t
{
    ZXIC_UINT32 fractional_nanosecond_of_hw_clock_cycle;
}DPP_PTPTM_PTPTM_HW_CLOCK_CYCLE_FRACTION_T;

typedef struct dpp_ptptm_ptptm_ptp_clock_cycle_integer_t
{
    ZXIC_UINT32 integeral_nanosecond_of_ptp_clock_cycle;
}DPP_PTPTM_PTPTM_PTP_CLOCK_CYCLE_INTEGER_T;

typedef struct dpp_ptptm_ptptm_ptp_clock_cycle_fraction_t
{
    ZXIC_UINT32 fractional_nanosecond_of_ptp_clock_cycle;
}DPP_PTPTM_PTPTM_PTP_CLOCK_CYCLE_FRACTION_T;

typedef struct dpp_ptptm_ptptm_ptp_configuration_t
{
    ZXIC_UINT32 trig_oe;
    ZXIC_UINT32 hw_time_update_en;
    ZXIC_UINT32 ptp1588_tod_time_update_en;
    ZXIC_UINT32 timer_enable;
    ZXIC_UINT32 pps_output_enable;
    ZXIC_UINT32 pp1_output_enable;
    ZXIC_UINT32 pp2_output_enable;
    ZXIC_UINT32 enable_writing_timestamps_to_the_fifo;
    ZXIC_UINT32 l2s_time_output_select;
    ZXIC_UINT32 reserved_9;
    ZXIC_UINT32 pps_input_select;
    ZXIC_UINT32 pp_output_select;
    ZXIC_UINT32 reserved_6;
    ZXIC_UINT32 timer_run_mode;
    ZXIC_UINT32 update_command_select;
    ZXIC_UINT32 trigger_out_enable;
    ZXIC_UINT32 trigger_in_enable;
    ZXIC_UINT32 timer_capture_slave_mode;
}DPP_PTPTM_PTPTM_PTP_CONFIGURATION_T;

typedef struct dpp_ptptm_ptptm_timer_control_t
{
    ZXIC_UINT32 ptpmoutputsynchroningstate;
    ZXIC_UINT32 ptp1588_fifo_read_command;
    ZXIC_UINT32 adjust_the_timer;
}DPP_PTPTM_PTPTM_TIMER_CONTROL_T;

typedef struct dpp_ptptm_ptptm_pps_income_delay_t
{
    ZXIC_UINT32 pps_income_delay_nanosecond;
    ZXIC_UINT32 pps_income_delay_frac_nanosecond;
}DPP_PTPTM_PTPTM_PPS_INCOME_DELAY_T;

typedef struct dpp_ptptm_ptptm_clock_cycle_update_t
{
    ZXIC_UINT32 tsn3_clock_cycle_update_enable;
    ZXIC_UINT32 tsn2_clock_cycle_update_enable;
    ZXIC_UINT32 tsn1_clock_cycle_update_enable;
    ZXIC_UINT32 tsn0_clock_cycle_update_enable;
    ZXIC_UINT32 ptp1588_clock_cycle_update_enable;
}DPP_PTPTM_PTPTM_CLOCK_CYCLE_UPDATE_T;

typedef struct dpp_ptptm_ptptm_cycle_time_of_output_period_pulse_1_t
{
    ZXIC_UINT32 clock_number_of_output_period_pulse_1;
}DPP_PTPTM_PTPTM_CYCLE_TIME_OF_OUTPUT_PERIOD_PULSE_1_T;

typedef struct dpp_ptptm_ptptm_cycle_time_of_output_period_pulse_2_t
{
    ZXIC_UINT32 clock_number_of_output_period_pulse_2;
}DPP_PTPTM_PTPTM_CYCLE_TIME_OF_OUTPUT_PERIOD_PULSE_2_T;

typedef struct dpp_ptptm_ptptm_timer_latch_en_t
{
    ZXIC_UINT32 latch_the_timer_en;
}DPP_PTPTM_PTPTM_TIMER_LATCH_EN_T;

typedef struct dpp_ptptm_ptptm_timer_latch_sel_t
{
    ZXIC_UINT32 timer_latch_sel;
}DPP_PTPTM_PTPTM_TIMER_LATCH_SEL_T;

typedef struct dpp_ptptm_ptptm_trigger_in_tod_nanosecond_t
{
    ZXIC_UINT32 trigger_in_tod_nanosecond;
}DPP_PTPTM_PTPTM_TRIGGER_IN_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_trigger_in_lower_tod_second_t
{
    ZXIC_UINT32 trigger_in_lower_tod_second;
}DPP_PTPTM_PTPTM_TRIGGER_IN_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_trigger_in_high_tod_second_t
{
    ZXIC_UINT32 trigger_in_high_tod_second;
}DPP_PTPTM_PTPTM_TRIGGER_IN_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_trigger_in_fracnanosecond_t
{
    ZXIC_UINT32 trigger_in_fracnanosecond;
}DPP_PTPTM_PTPTM_TRIGGER_IN_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_trigger_in_hardware_time_low_t
{
    ZXIC_UINT32 trigger_in_hardware_time_low;
}DPP_PTPTM_PTPTM_TRIGGER_IN_HARDWARE_TIME_LOW_T;

typedef struct dpp_ptptm_ptptm_trigger_in_hardware_time_high_t
{
    ZXIC_UINT32 trigger_in_hardware_time_high;
}DPP_PTPTM_PTPTM_TRIGGER_IN_HARDWARE_TIME_HIGH_T;

typedef struct dpp_ptptm_ptptm_trigger_out_tod_nanosecond_t
{
    ZXIC_UINT32 trigger_out_tod_nanosecond;
}DPP_PTPTM_PTPTM_TRIGGER_OUT_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_trigger_out_lower_tod_second_t
{
    ZXIC_UINT32 trigger_out_lower_tod_second;
}DPP_PTPTM_PTPTM_TRIGGER_OUT_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_trigger_out_high_tod_second_t
{
    ZXIC_UINT32 trigger_out_high_tod_second;
}DPP_PTPTM_PTPTM_TRIGGER_OUT_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_trigger_out_hardware_time_low_t
{
    ZXIC_UINT32 trigger_out_hardware_time_low;
}DPP_PTPTM_PTPTM_TRIGGER_OUT_HARDWARE_TIME_LOW_T;

typedef struct dpp_ptptm_ptptm_trigger_out_hardware_time_high_t
{
    ZXIC_UINT32 trigger_out_hardware_time_high;
}DPP_PTPTM_PTPTM_TRIGGER_OUT_HARDWARE_TIME_HIGH_T;

typedef struct dpp_ptptm_ptptm_adjust_tod_nanosecond_t
{
    ZXIC_UINT32 adjust_tod_nanosecond;
}DPP_PTPTM_PTPTM_ADJUST_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_adjust_lower_tod_second_t
{
    ZXIC_UINT32 adjust_lower_tod_second;
}DPP_PTPTM_PTPTM_ADJUST_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_adjust_high_tod_second_t
{
    ZXIC_UINT32 adjust_high_tod_second;
}DPP_PTPTM_PTPTM_ADJUST_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_adjust_fracnanosecond_t
{
    ZXIC_UINT32 adjust_fracnanosecond;
}DPP_PTPTM_PTPTM_ADJUST_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_adjust_hardware_time_low_t
{
    ZXIC_UINT32 adjust_hardware_time_low;
}DPP_PTPTM_PTPTM_ADJUST_HARDWARE_TIME_LOW_T;

typedef struct dpp_ptptm_ptptm_adjust_hardware_time_high_t
{
    ZXIC_UINT32 adjust_hardware_time_high;
}DPP_PTPTM_PTPTM_ADJUST_HARDWARE_TIME_HIGH_T;

typedef struct dpp_ptptm_ptptm_latch_tod_nanosecond_t
{
    ZXIC_UINT32 latch_tod_nanosecond;
}DPP_PTPTM_PTPTM_LATCH_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_latch_lower_tod_second_t
{
    ZXIC_UINT32 latch_lower_tod_second;
}DPP_PTPTM_PTPTM_LATCH_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_latch_high_tod_second_t
{
    ZXIC_UINT32 latch_high_tod_second;
}DPP_PTPTM_PTPTM_LATCH_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_latch_fracnanosecond_t
{
    ZXIC_UINT32 latch_fracnanosecond;
}DPP_PTPTM_PTPTM_LATCH_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_latch_hardware_time_low_t
{
    ZXIC_UINT32 latch_hardware_time_low;
}DPP_PTPTM_PTPTM_LATCH_HARDWARE_TIME_LOW_T;

typedef struct dpp_ptptm_ptptm_latch_hardware_time_high_t
{
    ZXIC_UINT32 latch_hardware_time_high;
}DPP_PTPTM_PTPTM_LATCH_HARDWARE_TIME_HIGH_T;

typedef struct dpp_ptptm_ptptm_real_tod_nanosecond_t
{
    ZXIC_UINT32 real_tod_nanosecond;
}DPP_PTPTM_PTPTM_REAL_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_real_lower_tod_second_t
{
    ZXIC_UINT32 real_lower_tod_second;
}DPP_PTPTM_PTPTM_REAL_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_real_high_tod_second_t
{
    ZXIC_UINT32 real_high_tod_second;
}DPP_PTPTM_PTPTM_REAL_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_real_hardware_time_low_t
{
    ZXIC_UINT32 real_hardware_time_low;
}DPP_PTPTM_PTPTM_REAL_HARDWARE_TIME_LOW_T;

typedef struct dpp_ptptm_ptptm_real_hardware_time_high_t
{
    ZXIC_UINT32 real_hardware_time_high;
}DPP_PTPTM_PTPTM_REAL_HARDWARE_TIME_HIGH_T;

typedef struct dpp_ptptm_ptptm_ptp1588_event_message_port_t
{
    ZXIC_UINT32 ptp1588_event_message_port;
}DPP_PTPTM_PTPTM_PTP1588_EVENT_MESSAGE_PORT_T;

typedef struct dpp_ptptm_ptptm_ptp1588_event_message_timestamp_low_t
{
    ZXIC_UINT32 ptp1588_event_message_timestamp_low;
}DPP_PTPTM_PTPTM_PTP1588_EVENT_MESSAGE_TIMESTAMP_LOW_T;

typedef struct dpp_ptptm_ptptm_ptp1588_event_message_timestamp_high_t
{
    ZXIC_UINT32 ptp1588_event_message_timestamp_high;
}DPP_PTPTM_PTPTM_PTP1588_EVENT_MESSAGE_TIMESTAMP_HIGH_T;

typedef struct dpp_ptptm_ptptm_ptp1588_event_message_fifo_status_t
{
    ZXIC_UINT32 fifo_full;
    ZXIC_UINT32 fifo_empty;
    ZXIC_UINT32 timestamps_count;
}DPP_PTPTM_PTPTM_PTP1588_EVENT_MESSAGE_FIFO_STATUS_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tod_nanosecond_t
{
    ZXIC_UINT32 latch_1588tod_nanosecond;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_lower_tod_second_t
{
    ZXIC_UINT32 latch_lower_1588tod_second;
}DPP_PTPTM_PTPTM_PP1S_LATCH_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_high_tod_second_t
{
    ZXIC_UINT32 latch_high_1588tod_second;
}DPP_PTPTM_PTPTM_PP1S_LATCH_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_fracnanosecond_t
{
    ZXIC_UINT32 latch_1588fracnanosecond;
}DPP_PTPTM_PTPTM_PP1S_LATCH_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn_time_configuration_t
{
    ZXIC_UINT32 tsn_pps_enable;
    ZXIC_UINT32 tsn_timer_enable;
    ZXIC_UINT32 tsn_timer_run_mode;
    ZXIC_UINT32 timer_capture_slave_mode;
}DPP_PTPTM_PTPTM_TSN_TIME_CONFIGURATION_T;

typedef struct dpp_ptptm_ptptm_tsn_timer_control_t
{
    ZXIC_UINT32 adjust_the_tsn3_timer;
    ZXIC_UINT32 adjust_the_tsn2_timer;
    ZXIC_UINT32 adjust_the_tsn1_timer;
    ZXIC_UINT32 adjust_the_tsn0_timer;
}DPP_PTPTM_PTPTM_TSN_TIMER_CONTROL_T;

typedef struct dpp_ptptm_ptptm_tsn0_clock_cycle_integer_t
{
    ZXIC_UINT32 integeral_nanosecond_of_tsn0_clock_cycle;
}DPP_PTPTM_PTPTM_TSN0_CLOCK_CYCLE_INTEGER_T;

typedef struct dpp_ptptm_ptptm_tsn0_clock_cycle_fraction_t
{
    ZXIC_UINT32 fractional_nanosecond_of_tsn0_clock_cycle;
}DPP_PTPTM_PTPTM_TSN0_CLOCK_CYCLE_FRACTION_T;

typedef struct dpp_ptptm_ptptm_tsn1_clock_cycle_integer_t
{
    ZXIC_UINT32 integeral_nanosecond_of_tsn1_clock_cycle;
}DPP_PTPTM_PTPTM_TSN1_CLOCK_CYCLE_INTEGER_T;

typedef struct dpp_ptptm_ptptm_tsn1_clock_cycle_fraction_t
{
    ZXIC_UINT32 fractional_nanosecond_of_tsn1_clock_cycle;
}DPP_PTPTM_PTPTM_TSN1_CLOCK_CYCLE_FRACTION_T;

typedef struct dpp_ptptm_ptptm_tsn2_clock_cycle_integer_t
{
    ZXIC_UINT32 integeral_nanosecond_of_tsn2_clock_cycle;
}DPP_PTPTM_PTPTM_TSN2_CLOCK_CYCLE_INTEGER_T;

typedef struct dpp_ptptm_ptptm_tsn2_clock_cycle_fraction_t
{
    ZXIC_UINT32 fractional_nanosecond_of_tsn2_clock_cycle;
}DPP_PTPTM_PTPTM_TSN2_CLOCK_CYCLE_FRACTION_T;

typedef struct dpp_ptptm_ptptm_tsn3_clock_cycle_integer_t
{
    ZXIC_UINT32 integeral_nanosecond_of_tsn3_clock_cycle;
}DPP_PTPTM_PTPTM_TSN3_CLOCK_CYCLE_INTEGER_T;

typedef struct dpp_ptptm_ptptm_tsn3_clock_cycle_fraction_t
{
    ZXIC_UINT32 fractional_nanosecond_of_tsn3_clock_cycle;
}DPP_PTPTM_PTPTM_TSN3_CLOCK_CYCLE_FRACTION_T;

typedef struct dpp_ptptm_ptptm_tsn0_adjust_tod_nanosecond_t
{
    ZXIC_UINT32 tsn0_adjust_tod_nanosecond;
}DPP_PTPTM_PTPTM_TSN0_ADJUST_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn0_adjust_lower_tod_second_t
{
    ZXIC_UINT32 tsn0_adjust_lower_tod_second;
}DPP_PTPTM_PTPTM_TSN0_ADJUST_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn0_adjust_high_tod_second_t
{
    ZXIC_UINT32 tsn0_adjust_high_tod_second;
}DPP_PTPTM_PTPTM_TSN0_ADJUST_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn0_adjust_fracnanosecond_t
{
    ZXIC_UINT32 tsn0_adjust_fracnanosecond;
}DPP_PTPTM_PTPTM_TSN0_ADJUST_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn1_adjust_tod_nanosecond_t
{
    ZXIC_UINT32 tsn1_adjust_tod_nanosecond;
}DPP_PTPTM_PTPTM_TSN1_ADJUST_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn1_adjust_lower_tod_second_t
{
    ZXIC_UINT32 tsn1_adjust_lower_tod_second;
}DPP_PTPTM_PTPTM_TSN1_ADJUST_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn1_adjust_high_tod_second_t
{
    ZXIC_UINT32 tsn1_adjust_high_tod_second;
}DPP_PTPTM_PTPTM_TSN1_ADJUST_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn1_adjust_fracnanosecond_t
{
    ZXIC_UINT32 tsn1_adjust_fracnanosecond;
}DPP_PTPTM_PTPTM_TSN1_ADJUST_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn2_adjust_tod_nanosecond_t
{
    ZXIC_UINT32 tsn2_adjust_tod_nanosecond;
}DPP_PTPTM_PTPTM_TSN2_ADJUST_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn2_adjust_lower_tod_second_t
{
    ZXIC_UINT32 tsn2_adjust_lower_tod_second;
}DPP_PTPTM_PTPTM_TSN2_ADJUST_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn2_adjust_high_tod_second_t
{
    ZXIC_UINT32 tsn2_adjust_high_tod_second;
}DPP_PTPTM_PTPTM_TSN2_ADJUST_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn2_adjust_fracnanosecond_t
{
    ZXIC_UINT32 tsn2_adjust_fracnanosecond;
}DPP_PTPTM_PTPTM_TSN2_ADJUST_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn3_adjust_tod_nanosecond_t
{
    ZXIC_UINT32 tsn3_adjust_tod_nanosecond;
}DPP_PTPTM_PTPTM_TSN3_ADJUST_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn3_adjust_lower_tod_second_t
{
    ZXIC_UINT32 tsn3_adjust_lower_tod_second;
}DPP_PTPTM_PTPTM_TSN3_ADJUST_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn3_adjust_high_tod_second_t
{
    ZXIC_UINT32 tsn3_adjust_high_tod_second;
}DPP_PTPTM_PTPTM_TSN3_ADJUST_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn3_adjust_fracnanosecond_t
{
    ZXIC_UINT32 tsn3_adjust_fracnanosecond;
}DPP_PTPTM_PTPTM_TSN3_ADJUST_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn0_latch_tod_nanosecond_t
{
    ZXIC_UINT32 tsn0_latch_tod_nanosecond;
}DPP_PTPTM_PTPTM_TSN0_LATCH_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn0_latch_lower_tod_second_t
{
    ZXIC_UINT32 tsn0_latch_lower_tod_second;
}DPP_PTPTM_PTPTM_TSN0_LATCH_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn0_latch_high_tod_second_t
{
    ZXIC_UINT32 tsn0_latch_high_tod_second;
}DPP_PTPTM_PTPTM_TSN0_LATCH_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn0_latch_fracnanosecond_t
{
    ZXIC_UINT32 tsn0_latch_fracnanosecond;
}DPP_PTPTM_PTPTM_TSN0_LATCH_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn1_latch_tod_nanosecond_t
{
    ZXIC_UINT32 tsn1_latch_tod_nanosecond;
}DPP_PTPTM_PTPTM_TSN1_LATCH_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn1_latch_lower_tod_second_t
{
    ZXIC_UINT32 tsn1_latch_lower_tod_second;
}DPP_PTPTM_PTPTM_TSN1_LATCH_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn1_latch_high_tod_second_t
{
    ZXIC_UINT32 tsn1_latch_high_tod_second;
}DPP_PTPTM_PTPTM_TSN1_LATCH_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn1_latch_fracnanosecond_t
{
    ZXIC_UINT32 tsn1_latch_fracnanosecond;
}DPP_PTPTM_PTPTM_TSN1_LATCH_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn2_latch_tod_nanosecond_t
{
    ZXIC_UINT32 tsn2_latch_tod_nanosecond;
}DPP_PTPTM_PTPTM_TSN2_LATCH_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn2_latch_lower_tod_second_t
{
    ZXIC_UINT32 tsn2_latch_lower_tod_second;
}DPP_PTPTM_PTPTM_TSN2_LATCH_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn2_latch_high_tod_second_t
{
    ZXIC_UINT32 tsn2_latch_high_tod_second;
}DPP_PTPTM_PTPTM_TSN2_LATCH_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn2_latch_fracnanosecond_t
{
    ZXIC_UINT32 tsn2_latch_fracnanosecond;
}DPP_PTPTM_PTPTM_TSN2_LATCH_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn3_latch_tod_nanosecond_t
{
    ZXIC_UINT32 tsn3_latch_tod_nanosecond;
}DPP_PTPTM_PTPTM_TSN3_LATCH_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn3_latch_lower_tod_second_t
{
    ZXIC_UINT32 tsn3_latch_lower_tod_second;
}DPP_PTPTM_PTPTM_TSN3_LATCH_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn3_latch_high_tod_second_t
{
    ZXIC_UINT32 tsn3_latch_high_tod_second;
}DPP_PTPTM_PTPTM_TSN3_LATCH_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn3_latch_fracnanosecond_t
{
    ZXIC_UINT32 tsn3_latch_fracnanosecond;
}DPP_PTPTM_PTPTM_TSN3_LATCH_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn0_tod_nanosecond_t
{
    ZXIC_UINT32 latch_tsn0_tod_nanosecond;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN0_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn0_lower_tod_second_t
{
    ZXIC_UINT32 latch_lower_tsn0_tod_second;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN0_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn0_high_tod_second_t
{
    ZXIC_UINT32 latch_high_tsn0_tod_second;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN0_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn0_fracnanosecond_t
{
    ZXIC_UINT32 latch_tsn0_fracnanosecond;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN0_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn1_tod_nanosecond_t
{
    ZXIC_UINT32 latch_tsn1_tod_nanosecond;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN1_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn1_lower_tod_second_t
{
    ZXIC_UINT32 latch_lower_tsn1_tod_second;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN1_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn1_high_tod_second_t
{
    ZXIC_UINT32 latch_high_tsn1_tod_second;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN1_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn1_fracnanosecond_t
{
    ZXIC_UINT32 latch_tsn1_fracnanosecond;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN1_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn2_tod_nanosecond_t
{
    ZXIC_UINT32 latch_tsn2_tod_nanosecond;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN2_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn2_lower_tod_second_t
{
    ZXIC_UINT32 latch_lower_tsn2_tod_second;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN2_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn2_high_tod_second_t
{
    ZXIC_UINT32 latch_high_tsn2_tod_second;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN2_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn2_fracnanosecond_t
{
    ZXIC_UINT32 latch_tsn2_fracnanosecond;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN2_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn3_tod_nanosecond_t
{
    ZXIC_UINT32 latch_tsn3_tod_nanosecond;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN3_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn3_lower_tod_second_t
{
    ZXIC_UINT32 latch_lower_tsn3_tod_second;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN3_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn3_high_tod_second_t
{
    ZXIC_UINT32 latch_high_tsn3_tod_second;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN3_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_pp1s_latch_tsn3_fracnanosecond_t
{
    ZXIC_UINT32 latch_tsn3_fracnanosecond;
}DPP_PTPTM_PTPTM_PP1S_LATCH_TSN3_FRACNANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn0_real_tod_nanosecond_t
{
    ZXIC_UINT32 tsn0_real_tod_nanosecond;
}DPP_PTPTM_PTPTM_TSN0_REAL_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn0_real_lower_tod_second_t
{
    ZXIC_UINT32 tsn0_real_lower_tod_second;
}DPP_PTPTM_PTPTM_TSN0_REAL_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn0_real_high_tod_second_t
{
    ZXIC_UINT32 tsn0_real_high_tod_second;
}DPP_PTPTM_PTPTM_TSN0_REAL_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn1_real_tod_nanosecond_t
{
    ZXIC_UINT32 tsn1_real_tod_nanosecond;
}DPP_PTPTM_PTPTM_TSN1_REAL_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn1_real_lower_tod_second_t
{
    ZXIC_UINT32 tsn1_real_lower_tod_second;
}DPP_PTPTM_PTPTM_TSN1_REAL_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn1_real_high_tod_second_t
{
    ZXIC_UINT32 tsn1_real_high_tod_second;
}DPP_PTPTM_PTPTM_TSN1_REAL_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn2_real_tod_nanosecond_t
{
    ZXIC_UINT32 tsn2_real_tod_nanosecond;
}DPP_PTPTM_PTPTM_TSN2_REAL_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn2_real_lower_tod_second_t
{
    ZXIC_UINT32 tsn2_real_lower_tod_second;
}DPP_PTPTM_PTPTM_TSN2_REAL_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn2_real_high_tod_second_t
{
    ZXIC_UINT32 tsn2_real_high_tod_second;
}DPP_PTPTM_PTPTM_TSN2_REAL_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn3_real_tod_nanosecond_t
{
    ZXIC_UINT32 tsn3_real_tod_nanosecond;
}DPP_PTPTM_PTPTM_TSN3_REAL_TOD_NANOSECOND_T;

typedef struct dpp_ptptm_ptptm_tsn3_real_lower_tod_second_t
{
    ZXIC_UINT32 tsn3_real_lower_tod_second;
}DPP_PTPTM_PTPTM_TSN3_REAL_LOWER_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_tsn3_real_high_tod_second_t
{
    ZXIC_UINT32 tsn3_real_high_tod_second;
}DPP_PTPTM_PTPTM_TSN3_REAL_HIGH_TOD_SECOND_T;

typedef struct dpp_ptptm_ptptm_real_ptp_clock_cycle_integer_t
{
    ZXIC_UINT32 integeral_nanosecond_of_real_ptp_clock_cycle;
}DPP_PTPTM_PTPTM_REAL_PTP_CLOCK_CYCLE_INTEGER_T;

typedef struct dpp_ptptm_ptptm_real_ptp_clock_cycle_fraction_t
{
    ZXIC_UINT32 fractional_nanosecond_of_real_ptp_clock_cycle;
}DPP_PTPTM_PTPTM_REAL_PTP_CLOCK_CYCLE_FRACTION_T;

typedef struct dpp_ptptm_ptptm_real_tsn0_clock_cycle_integer_t
{
    ZXIC_UINT32 integeral_nanosecond_of_real_tsn0_clock_cycle;
}DPP_PTPTM_PTPTM_REAL_TSN0_CLOCK_CYCLE_INTEGER_T;

typedef struct dpp_ptptm_ptptm_real_tsn0_clock_cycle_fraction_t
{
    ZXIC_UINT32 fractional_nanosecond_of_real_tsn0_clock_cycle;
}DPP_PTPTM_PTPTM_REAL_TSN0_CLOCK_CYCLE_FRACTION_T;

typedef struct dpp_ptptm_ptptm_real_tsn1_clock_cycle_integer_t
{
    ZXIC_UINT32 integeral_nanosecond_of_real_tsn1_clock_cycle;
}DPP_PTPTM_PTPTM_REAL_TSN1_CLOCK_CYCLE_INTEGER_T;

typedef struct dpp_ptptm_ptptm_real_tsn1_clock_cycle_fraction_t
{
    ZXIC_UINT32 fractional_nanosecond_of_real_tsn1_clock_cycle;
}DPP_PTPTM_PTPTM_REAL_TSN1_CLOCK_CYCLE_FRACTION_T;

typedef struct dpp_ptptm_ptptm_real_tsn2_clock_cycle_integer_t
{
    ZXIC_UINT32 integeral_nanosecond_of_real_tsn2_clock_cycle;
}DPP_PTPTM_PTPTM_REAL_TSN2_CLOCK_CYCLE_INTEGER_T;

typedef struct dpp_ptptm_ptptm_real_tsn2_clock_cycle_fraction_t
{
    ZXIC_UINT32 fractional_nanosecond_of_real_tsn2_clock_cycle;
}DPP_PTPTM_PTPTM_REAL_TSN2_CLOCK_CYCLE_FRACTION_T;

typedef struct dpp_ptptm_ptptm_real_tsn3_clock_cycle_integer_t
{
    ZXIC_UINT32 integeral_nanosecond_of_real_tsn3_clock_cycle;
}DPP_PTPTM_PTPTM_REAL_TSN3_CLOCK_CYCLE_INTEGER_T;

typedef struct dpp_ptptm_ptptm_real_tsn3_clock_cycle_fraction_t
{
    ZXIC_UINT32 fractional_nanosecond_of_real_tsn3_clock_cycle;
}DPP_PTPTM_PTPTM_REAL_TSN3_CLOCK_CYCLE_FRACTION_T;


#ifdef __cplusplus
}
#endif
#endif

