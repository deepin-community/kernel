#ifndef _ZX_REGS_H
#define _ZX_REGS_H

/**************************** ptpm start **************************/

#define PTPM_OFFSET_WITH_TOP        0x4000

#define EXTERNAL_PPS_BIT            4
#define TRIGGER_OUT_BIT             1
#define TRIGGER_IN_BIT              0
/* bit4: pps_in_status
          0: not receive pps
          1: receive pps */
#define INTERRUPT_STATUS            0x00000010
/* bit4: pps income event, writing 1 to the bit clearing it
          0: not receive pps
          1: receive pps */
#define INTERRUPT_EVENT             0x00000014
/* bit4: pps income event mask
          0: mask
          1: no mask */
#define INTERRUPT_MASK              0x00000018
/* bit4: trigger pps income event test
          0: not occur
          1: occur   */
#define INTERRUPT_TEST              0x0000001C

/* adjust clock cycle, for 1588 and hw timer */
#define PTP_CLOCK_CYCLE_INTEGER     0x00000030
#define PTP_CLOCK_CYCLE_FRACTION    0x00000034

/* 
    BIT18: trig oe
           1: trig out
           0: trig in
    bit17: hw timer update enable, used when timer run mode is update,increment or decrement
           0: update disable
           1: update enable
    bit16: 1588 timer update enable, used when timer run mode is update,increment or decrement
           0: update disable
           1: update enable
    bit15: 1588 and hw time timer enable
           0: timer disable
           1: timer enable
    bit8:  pps input select
           0: select internal pps
           1: select external pps
    bit5:4 1588 tod and hw timer run mode
           0: normal
           1: update
           2: increment
           3: decrement
    bit2: trigger out enable
           0:disable
           1:enable
    bit1: trigger in enable
           0:disable
           1:enable
    bit0: 1588 tod timer slave/capture mode
           0:capture mode, capture 1588 tod timer when input pps pulse
           1:slave mode, the timer make the input pps as reference
*/
#define PTP_CONFIGURATION        0x00000040

#define PPS_TRIGGER_IN_BIT       1
#define PPS_TRIGGER_OUT_BIT      2
#define PPS_RUN_MODE_BIT         4
#define PPS_INPUT_SEL_BIT        8
#define TIMER_EN_BIT                   15
#define TIMER_1588_UPT_EN_BIT       16
#define TIMER_HW_UPT_EN_BIT   17
#define TRIG_OE                18
enum timer_run_mode {
    NORMAL_MODE,
    UPDATE_MODE,
    INCRE_MODE,
    DECRE_MODE,
};
/* bit1: adjust the 1588 tod and hw timer
         0: not adjust
         1: adjust the timer with add/sub/ or update
 */
#define TIMER_CONTROL            0x00000044
#define ADJ_TIMER_BIT            1

/*
   bit4: tsn3 clock cycle update enable
   bit3: tsn2 clock cycle update enable
   bit2: tsn1 clock cycle update enable
   bit1: tsn0 clock cycle update enable
   bit0: 1588 clock cycle update enable
*/
#define CLOCK_CYCLE_UPDATE       0x0000004C

/* bit31~16: nanosecond         0~0xffff
   bit15~0 : frac nanosecond    0~0xffff  */
#define PPS_INCOME_DELAY         0x00000048
/* bit0, 0:not latch, 1:latch the timer */
#define TIMER_LATCH_EN           0x00000058
/* bit5:0, select the latch timer 
   bit0: 1588 timer
   bit1: hw timer
   bit2: tsn0 timer
   bit3: tsn1 timer
   bit4: tsn2 timer
   bit5: tsn3 tiemr */
#define TIMER_LACTH_SEL           0x0000005C

enum latch_timer_type {
    LATCH_1588_TIMER,
    LATCH_HW_TIMER,
    LATCH_TSN0_TIMER,
    LATCH_TSN1_TIMER,
    LATCH_TSN2_TIMER,
    LATCH_TSN3_TIMER,
};

// trigger in 
#define TRIGGER_IN_TOD_NANO_SECOND    0x00000060
#define TRIGGER_IN_LOWER_TOD_SECOND   0x00000064
#define TRIGGER_IN_HIGH_TOD_SECOND    0x00000068
#define TRIGGER_IN_FRAC_NANO_SECOND   0x0000006C

#define TRIGGER_IN_HARDWARE_TIME_LOW  0x00000070
#define TRIGGER_IN_HARDWARE_TIME_HIGH 0x00000074

// trigger out
#define TRIGGER_OUT_TOD_NANO_SECOND    0x00000080
#define TRIGGER_OUT_LOWER_TOD_SECOND   0x00000084
#define TRIGGER_OUT_HIGH_TOD_SECOND    0x00000088
// #define TRIGGER_OUT_FRAC_NANO_SECOND   0x0000008C  // none

#define TRIGGER_OUT_HARDWARE_TIME_LOW  0x00000090
#define TRIGGER_OUT_HARDWARE_TIME_HIGH 0x00000094


/* adjust 1588 timer */
#define ADJUST_TOD_NANO_SECOND    0x000000A0
#define ADJUST_LOWER_TOD_SECOND   0x000000A4
#define ADJUST_HIGH_TOD_SECOND    0x000000A8
#define ADJUST_FRAC_NANO_SECOND   0x000000AC
/* adjust hardware timer */
#define ADJUST_HARDWARE_TIME_LOW  0x000000B0
#define ADJUST_HARDWARE_TIME_HIGH 0x000000B4

/* 1588 timer latched time */
#define LATCH_TOD_NANO_SECOND     0x000000C0
#define LATCH_LOWER_TOD_SECOND    0x000000C4
#define LATCH_HIGH_TOD_SECOND     0x000000C8
#define LATCH_FRAC_NANO_SECOND    0x000000CC
/* hw timer latched time */
#define LATCH_HARDWARE_TIME_LOW   0x000000D0
#define LATCH_HARDWARE_TIME_HIGH  0x000000D4

/* pps capture tod time*/
#define PPS_LATCH_TOD_NANO_SECOND  0x00000120
#define PPS_LATCH_LOWER_TOD_SECOND 0x00000124
#define PPS_LATCH_HIGH_TOD_SECOND  0x00000128
#define PPS_LATCH_FRAC_NANO_SECOND 0x0000012C

/*  bit19~16 tsn pps enable
           bit19: tsn3
           bit18: tsn2
           bit17: tsn1
           bit16: tsn0
    bit15~12 tsn timer enable
           bit15: tsn3
           bit14: tsn2
           bit13: tsn1
           bit12: tsn0
    bit11~10 tsn0 timer run mode
    bit9~8   tsn1 timer run mode, 
    bit7~6   tsn2 timer run mode, 
    bit5~4   tsn3 timer run mode, 
            0: normal
            1: update
            2: increment
            3: decrement
    bit3~0: tsn timer slave/capture mode, 0 is capture mode, 1 is slave mode
            bit3: tsn3
            bit2: tsn2
            bit1: tsn1
            bit0: tsn0
*/
#define TSN_TIME_CONFIGURATION        0x00000140
/*
    bit3: adjust tsn3 timer
    bit2: adjust tsn2 timer
    bit1: adjust tsn1 timer
    bit0: adjust tsn0 timer
*/
#define TSN_TIMER_CONTROL             0x00000144
#define TSN0_ADJ_EN_BIT               0
#define TSN1_ADJ_EN_BIT               1
#define TSN2_ADJ_EN_BIT               2
#define TSN3_ADJ_EN_BIT               3

/* adjust clock cycle, for four tsn timer */
#define TSN0_CLOCK_CYCLE_INTEGER      0x00000148
#define TSN0_CLOCK_CYCLE_FRACTION     0x0000014C
#define TSN1_CLOCK_CYCLE_INTEGER      0x00000150
#define TSN1_CLOCK_CYCLE_FRACTION     0x00000154
#define TSN2_CLOCK_CYCLE_INTEGER      0x00000158
#define TSN2_CLOCK_CYCLE_FRACTION     0x0000015C
#define TSN3_CLOCK_CYCLE_INTEGER      0x00000160
#define TSN3_CLOCK_CYCLE_FRACTION     0x00000164

#define TSN_CLOCK_CYCLE_INTEGER(tsn_no)     TSN##tsn_no##_CLOCK_CYCLE_INTEGER
#define TSN_CLOCK_CYCLE_FRACTION(tsn_no)    TSN##tsn_no##_CLOCK_CYCLE_FRACTION

/* adjust tsn timer */
#define TSN0_ADJUST_TOD_NANO_SECOND   0x00000180
#define TSN0_ADJUST_LOWER_TOD_SECOND  0x00000184
#define TSN0_ADJUST_HIGH_TOD_SECOND   0x00000188
#define TSN0_ADJUST_FRAC_NANO_SECOND  0x0000018C

#define TSN1_ADJUST_TOD_NANO_SECOND   0x00000190
#define TSN1_ADJUST_LOWER_TOD_SECOND  0x00000194
#define TSN1_ADJUST_HIGH_TOD_SECOND   0x00000198
#define TSN1_ADJUST_FRAC_NANO_SECOND  0x0000019C

#define TSN2_ADJUST_TOD_NANO_SECOND   0x000001A0
#define TSN2_ADJUST_LOWER_TOD_SECOND  0x000001A4
#define TSN2_ADJUST_HIGH_TOD_SECOND   0x000001A8
#define TSN2_ADJUST_FRAC_NANO_SECOND  0x000001AC

#define TSN3_ADJUST_TOD_NANO_SECOND   0x000001B0
#define TSN3_ADJUST_LOWER_TOD_SECOND  0x000001B4
#define TSN3_ADJUST_HIGH_TOD_SECOND   0x000001B8
#define TSN3_ADJUST_FRAC_NANO_SECOND  0x000001BC

#define TSN_ADJUST_NANO_SEC(tsn_no)       TSN##tsn_no##_ADJUST_TOD_NANO_SECOND
#define TSN_ADJUST_LOW_SECOND(tsn_no)     TSN##tsn_no##_ADJUST_LOWER_TOD_SECOND
#define TSN_ADJUST_HIGH_SECOND(tsn_no)    TSN##tsn_no##_ADJUST_HIGH_TOD_SECOND
#define TSN_ADJUST_FRAC_NANO_SEC(tsn_no)  TSN##tsn_no##_ADJUST_FRAC_NANO_SECOND

/* tsn0 timer latched time */
#define TSN0_LATCH_TOD_NANO_SECOND    0x000001C0
#define TSN0_LATCH_LOWER_TOD_SECOND   0x000001C4
#define TSN0_LATCH_HIGH_TOD_SECOND    0x000001C8
#define TSN0_LATCH_FRAC_NANO_SECOND   0x000001CC
/* tsn1 timer latched time */
#define TSN1_LATCH_TOD_NANO_SECOND    0x000001D0
#define TSN1_LATCH_LOWER_TOD_SECOND   0x000001D4
#define TSN1_LATCH_HIGH_TOD_SECOND    0x000001D8
#define TSN1_LATCH_FRAC_NANO_SECOND   0x000001DC
/* tsn2 timer latched time */
#define TSN2_LATCH_TOD_NANO_SECOND    0x000001E0
#define TSN2_LATCH_LOWER_TOD_SECOND   0x000001E4
#define TSN2_LATCH_HIGH_TOD_SECOND    0x000001E8
#define TSN2_LATCH_FRAC_NANO_SECOND   0x000001EC
/* tsn3 timer latched time */
#define TSN3_LATCH_TOD_NANO_SECOND    0x000001F0
#define TSN3_LATCH_LOWER_TOD_SECOND   0x000001F4
#define TSN3_LATCH_HIGH_TOD_SECOND    0x000001F8
#define TSN3_LATCH_FRAC_NANO_SECOND   0x000001FC

#define TSN_LATCH_NANO_SEC(tsn_no)        TSN##tsn_no##_LATCH_TOD_NANO_SECOND
#define TSN_LATCH_LOW_SECOND(tsn_no)      TSN##tsn_no##_LATCH_LOWER_TOD_SECOND
#define TSN_LATCH_HIGH_SECOND(tsn_no)     TSN##tsn_no##_LATCH_HIGH_TOD_SECOND
#define TSN_LATCH_FRAC_NANO_SEC(tsn_no)   TSN##tsn_no##_LATCH_FRAC_NANO_SECOND

/* pps capture tsn0 time*/
#define PPS_LATCH_TSN0_NANO_SECOND  0x00000200
#define PPS_LATCH_TSN0_LOWER_SECOND 0x00000204
#define PPS_LATCH_TSN0_HIGH_SECOND  0x00000208
#define PPS_LATCH_TSN0_FRAC_NANO    0x0000020C
/* pps capture tsn1 time*/
#define PPS_LATCH_TSN1_NANO_SECOND  0x00000210
#define PPS_LATCH_TSN1_LOWER_SECOND 0x00000214
#define PPS_LATCH_TSN1_HIGH_SECOND  0x00000218
#define PPS_LATCH_TSN1_FRAC_NANO    0x0000021C
/* pps capture tsn2 time*/
#define PPS_LATCH_TSN2_NANO_SECOND  0x00000220
#define PPS_LATCH_TSN2_LOWER_SECOND 0x00000224
#define PPS_LATCH_TSN2_HIGH_SECOND  0x00000228
#define PPS_LATCH_TSN2_FRAC_NANO    0x0000022C
/* pps capture tsn3 time*/
#define PPS_LATCH_TSN3_NANO_SECOND  0x00000230
#define PPS_LATCH_TSN3_LOWER_SECOND 0x00000234
#define PPS_LATCH_TSN3_HIGH_SECOND  0x00000238
#define PPS_LATCH_TSN3_FRAC_NANO    0x0000023C

#define PPS_LATCH_TSN_NANO_SEC(tsn_no)        PPS_LATCH_TSN##tsn_no##_NANO_SECOND
#define PPS_LATCH_TSN_LOW_SECOND(tsn_no)      PPS_LATCH_TSN##tsn_no##_LOWER_SECOND
#define PPS_LATCH_TSN_HIGH_SECOND(tsn_no)     PPS_LATCH_TSN##tsn_no##_HIGH_SECOND
#define PPS_LATCH_TSN_FRAC_NANO_SEC(tsn_no)   PPS_LATCH_TSN##tsn_no##_FRAC_NANO

/**************************** ptpm end **************************/


/**************************** ptp_top start **************************/

/*  bit3: local pp1s status
        0: not generate local pp1s
        1: generate local pp1s
    bit2: trigger local pp1s event test
        0: not occur
        1: occur
    bit1: writing 1 to the bit clearing local pp1s status
        0: not clear
        1: clear
    bit0: local pp1s event enable
        0: no enable
        1: enable */
#define LOCAL_PPS_INTERRUPT        0x00000000
// bit0:  0 ref0   1 ref1
#define PP1S_EXTERNAL_SEL          0x00000004
/*  bit1~0: select pp1s out:
        00: pp1s ref0
        01: pp1s ref1
        10: local pp1s
        11: 1588 pp1s  */
#define PP1S_OUT_SEL               0x00000008
/*  bit2~0: select test pp1s:
        000: pp1s ref0
        001: pp1s ref1
        010: local pp1s
        011: 1588 pp1s
        100: tsn0 pp1s
        101: tsn1 pp1s
        110: tsn2 pp1s
        111: tsn3 pp1s  */
#define TEST_PP1S_SEL              0x0000000C

#define LOCAL_PP1S_EN              0x00000010 // use local_pp1s need enable this reg
/*  bit2~1: select local pp1s adjust reference
             00: pp1s ref0
             01: pp1s ref1
             10,11: 1588 pp1s
    bit0:   adjust local pp1s phase once */
#define LOCAL_PP1S_ADJUST          0x00000014
/* bit29~0: adjust local pp1s value, must below 10e9, 1bit is 1 nanosecond */
#define LOCAL_PP1S_ADJUST_VALUE    0x00000018

/*  bit5~3: Pd_U1_Sel1
        000: pp1s ref0
        001: pp1s ref1
        010: local pp1s
        011: 1588 pp1s
        100: tsn0 pp1s
        101: tsn1 pp1s
        110: tsn2 pp1s
        111: tsn3 pp1s
    bit2~0: Pd_U1_Sel0
        000: pp1s ref0
        001: pp1s ref1
        010: local pp1s
        011: 1588 pp1s
        100: tsn0 pp1s
        101: tsn1 pp1s
        110: tsn2 pp1s
        111: tsn3 pp1s    */
#define PD_U1_SEL                  0x00000040
// bit29~0: phase detector module 1 select0 input pp1s shift value.
#define PD_U1_PD0_SHIFT            0x00000044
// bit29~0: phase detector module 1 select1 input pp1s shift value. 
#define PD_U1_PD1_SHIFT            0x00000048

/*  bit30: Pd_U1_Result_sign
        1: positive
        0: negative
    bit29: Pd_U1_Overflow: the interval between two pp1s pluse is greater than 0x1FFF_FFFF, 1 is overflow
    bit28~0: phase detector module 1 resule value, must below 5*10e8, 1 bit is 1 nanosecond. */
#define PD_U1_RESULT               0x0000004C

/*  bit5~3: Pd_U2_Sel1
        000: pp1s ref0
        001: pp1s ref1
        010: local pp1s
        011: 1588 pp1s
        100: tsn0 pp1s
        101: tsn1 pp1s
        110: tsn2 pp1s
        111: tsn3 pp1s 
    bit2~0: Pd_U2_Sel0
        000: pp1s ref0
        001: pp1s ref1
        010: local pp1s
        011: 1588 pp1s
        100: tsn0 pp1s
        101: tsn1 pp1s
        110: tsn2 pp1s
        111: tsn3 pp1s    */
#define PD_U2_SEL                  0x00000050
// bit29~0: phase detector module 2 select0 input pp1s shift value.
#define PD_U2_PD0_SHIFT            0x00000054
// bit29~0: phase detector module 2 select1 input pp1s shift value.
#define PD_U2_PD1_SHIFT            0x00000058
/*  bit30: Pd_U2_Result_sign
        1: positive
        0: negative
    bit29: Pd_U2_Overflow: the interval between two pp1s pluse is greater than 0x1FFF_FFFF, 1 is overflow
    bit28~0: phase detector module 2 resule value, must below 5*10e8, 1 bit is 1 nanosecond. */
#define PD_U2_RESULT               0x0000005C

#define TSN_GROUP_NANO_SEC_DELAY0            0x00000080
#define TSN_GROUP_FRAC_NANO_SEC_DELAY0       0x00000084
#define TSN_GROUP_NANO_SEC_DELAY1            0x00000088
#define TSN_GROUP_FRAC_NANO_SEC_DELAY1       0x0000008C
#define TSN_GROUP_NANO_SEC_DELAY2            0x00000090
#define TSN_GROUP_FRAC_NANO_SEC_DELAY2       0x00000094
#define TSN_GROUP_NANO_SEC_DELAY3            0x00000098
#define TSN_GROUP_FRAC_NANO_SEC_DELAY3       0x0000009C
#define PTP1588_RDMA_NANO_SEC_DELAY          0x000000A0
#define PTP1588_RDMA_FRAC_NANO_SEC_DELAY     0x000000A4
#define PTP1588_NP_NANO_SEC_DELAY            0x000000A8
#define PTP1588_NP_FRAC_NANO_SEC_DELAY       0x000000AC
#define PTP1588_NVME_NANO_SEC_DELAY1         0x000000C0
#define PTP1588_NVME_FRAC_NANO_SEC_DELAY1    0x000000C4
#define PTP1588_NVME_NANO_SEC_DELAY2         0x000000C8
#define PTP1588_NVME_FRAC_NANO_SEC_DELAY2    0x000000CC


/**************************** ptp_top end **************************/


/**************************** ptps start **************************/
/* bit0: enable writing timestamps to the FIFO
         0: disable
         1: enable */
#define PTPS_CONFIGURATION           0x00000020

/* bit0: PTP1588 FIFO read command
         0: not read
         1: read */
#define PTPS_TIMER_CONTROL                0x00000024

/* bit15~0: integral nanosecond of sync Hardware Time compensaion, 1 bit is 1 nanosecond */
#define SYNC_HW_TIME_COMPENSATION    0x00000038

/* 对于加密报文，先读 PTP event timestamps count，然后读时间戳和匹配信息，读时间戳和匹配信息之前，都要先配置TIMER_CONTROL.
   匹配信息是在PTP1588_EVENT_MESSAGE_TS_LOW里，具体对应关系: BIT[23:0]: messageType, sourcePortIdentity[3:0],sequenceId[15:0] */
/* ptp1588 event message timestamp[31:0] */
#define PTP1588_EVENT_MESSAGE_TS_LOW    0x00000084
/* ptp1588 event message timestamp[63:32] */
#define PTP1588_EVENT_MESSAGE_TS_HIGH   0x00000088

/* bit9: FIFO full
   bit8: FIFO empty
   bit7~0: PTP event timestamps count */
#define PTP1588_EVENT_MESSAGE_FIFO_STATUS   0x0000008C

struct event_ts_info
{
   u32 ts_low;
   u32 ts_high;
   unsigned char messageType;
   unsigned char srcPortId;
   short sequenceId;
};

#define EVENT_MESSAGE_MAX_NUM         32

/**************************** ptps end **************************/




#endif /* _ZX_REGS_H */
