#ifndef  _DDK770_DP_H_
#define  _DDK770_DP_H_

#include "ddk770_mode.h"
#include "ddk770_display.h"
#include "../hw_com.h"
#include "../smi_drv.h"

#define DP_SUCCESS                  0L
#define DP_FAILURE                  1L
#define DP_SCRAMBLE_ENABLE          1
#define DP_SCRAMBLE_DISABLE         0

#define DP_IIC_NUMBER               0
#define DP_IIC_BASE_ADDR            0x40800000
#define DP_IIC_VFMCEXP_0_ADDR       0x64
#define DP_IIC_VFMCEXP_1_ADDR       0x65
#define DP_IIC_MUX_ADDR             0x70
#define DP_IIC_TI_DP141_ADDR        0x5
#define DP_IIC_IDT8N49_ADDR         0x7C
#define DP_IIC_LMK03318_ADDR		0x50

#define DP_EDID_BUF_LEN             128     /** Per Block size, */

#define DP_IDT_8T49N24X_FIN_MIN     8000
#define DP_IDT_8T49N24X_FIN_MAX     875000000

#define DP_AUX_I2C_WRITE		0x0
#define DP_AUX_I2C_READ			0x1
#define DP_AUX_I2C_WRITE_STATUS_UPDATE	0x2
#define DP_AUX_I2C_MOT			0x4
#define DP_AUX_NATIVE_WRITE		0x8
#define DP_AUX_NATIVE_READ		0x9

#define DP_AUX_NATIVE_REPLY_ACK		0x0
#define DP_AUX_NATIVE_REPLY_NACK	0x1

#define DP_AUX_NATIVE_WAIT_REPLY		0x1
#define DP_AUX_NATIVE_RECEIVED_REPLY	0x0

#define DP_OFFSET 0x8000

// FROM INNO SDK
/* DPCD */
#define DP_DPCD_REV			0x000

#define DP_MAX_LINK_RATE                0x01

#define DP_MAX_LANE_COUNT               0x02 //    0x002

#define DP_MAX_DOWNSPREAD				0x03

#define DP_MAX_LANE_COUNT_MASK		    0x1f
#define DP_TPS3_SUPPORTED		    (1 << 6) /* 1.2 */
#define DP_ENHANCED_FRAME_CAP		    (1 << 7)

#define DP_TRAINING_AUX_RD_INTERVAL             0x00e   /* XXX 1.2? */
#define DP_TRAINING_AUX_RD_MASK                0x7F    /* DP 1.3 */
#define DP_EXTENDED_RECEIVER_CAP_FIELD_PRESENT (1 << 7) /* DP 1.3 */

#define DP_SET_POWER      0x600
#define DP_SET_POWER_D0   0x1
#define DP_SET_POWER_D3   0x2
#define DP_SET_POWER_MASK 0x3

#define DP_AUX_MAX_PAYLOAD_BYTES 16

#define DP_AUX_REPLY_ACK   (0x0)
#define DP_AUX_REPLY_NACK  (0x1)
#define DP_AUX_REPLY_DEFER (0x2)
#define DP_AUX_I2C_REPLY_NAKE  (0x4)
#define DP_AUX_I2C_REPLY_DEFER (0x8)

#define DP_LINK_STATUS_SIZE 6

// FROM INNO SDK
/* hpd */
#define DP_LINK_SERVICE_IRQ_VECTOR_ESI0 0x2005
#define DP_HPD_IRQ  (0x1)
#define DP_HPD_IN    (0x2)
#define DP_HPD_OUT (0x4)


// DP_PHY100
/* link configuration */
#define	DP_LINK_BW_SET		0x100
#define DP_LINK_BW_1_62 (0x06)
#define DP_LINK_BW_2_7  (0x0a)
#define DP_LINK_BW_5_4  (0x14)
#define DP_LINK_BW_8_1  (0x1e)

#define DP_LANE_COUNT_SET	0x101
#define DP_TRAINING_PATTERN_SET	        0x102
#define DP_TRAINING_PATTERN_DISABLE	    0
#define DP_TRAINING_PATTERN_1		    1
#define DP_TRAINING_PATTERN_2		    2
#define DP_TRAINING_PATTERN_2_CDS	    3	    /* 2.0 E11 */
#define DP_TRAINING_PATTERN_3		    3	    /* 1.2 */
#define DP_TRAINING_PATTERN_4           7       /* 1.4 */
#define DP_TRAINING_PATTERN_MASK	    0x3
#define DP_TRAINING_PATTERN_MASK_1_4	 0xf

/* DPCD 1.1 only. For DPCD >= 1.2 see per-lane DP_LINK_QUAL_LANEn_SET */
#define DP_LINK_QUAL_PATTERN_11_DISABLE    (0 << 2)
#define DP_LINK_QUAL_PATTERN_11_D10_2	    (1 << 2)
#define DP_LINK_QUAL_PATTERN_11_ERROR_RATE (2 << 2)
#define DP_LINK_QUAL_PATTERN_11_PRBS7	    (3 << 2)
#define DP_LINK_QUAL_PATTERN_11_MASK	    (3 << 2)

#define DP_RECOVERED_CLOCK_OUT_EN	    (1 << 4)
#define DP_LINK_SCRAMBLING_DISABLE	    (1 << 5)

#define DP_SYMBOL_ERROR_COUNT_BOTH	    (0 << 6)
#define DP_SYMBOL_ERROR_COUNT_DISPARITY    (1 << 6)
#define DP_SYMBOL_ERROR_COUNT_SYMBOL	    (2 << 6)
#define DP_SYMBOL_ERROR_COUNT_MASK	    (3 << 6)


#define DP_TRAINING_LANE0_SET		    0x103
#define DP_TRAINING_LANE1_SET		    0x104
#define DP_TRAINING_LANE2_SET		    0x105
#define DP_TRAINING_LANE3_SET		    0x106

#define DP_TRAIN_VOLTAGE_SWING_MASK	    0x3
#define DP_TRAIN_VOLTAGE_SWING_SHIFT	    0
#define DP_TRAIN_MAX_SWING_REACHED	    (1 << 2)
#define DP_TRAIN_VOLTAGE_SWING_LEVEL_0 (0 << 0)
#define DP_TRAIN_VOLTAGE_SWING_LEVEL_1 (1 << 0)
#define DP_TRAIN_VOLTAGE_SWING_LEVEL_2 (2 << 0)
#define DP_TRAIN_VOLTAGE_SWING_LEVEL_3 (3 << 0)

#define DP_TRAIN_PRE_EMPHASIS_MASK	    (3 << 3)
#define DP_TRAIN_PRE_EMPH_LEVEL_0		(0 << 3)
#define DP_TRAIN_PRE_EMPH_LEVEL_1		(1 << 3)
#define DP_TRAIN_PRE_EMPH_LEVEL_2		(2 << 3)
#define DP_TRAIN_PRE_EMPH_LEVEL_3		(3 << 3)

#define DP_DOWNSPREAD_CTRL		    0x107
#define DP_SPREAD_AMP_0_5		    (1 << 4)
#define DP_FIXED_VTOTAL_AS_SDP_EN_IN_PR_ACTIVE  (1 << 6)
#define DP_MSA_TIMING_PAR_IGNORE_EN	    (1 << 7) /* eDP */

#define DP_MAIN_LINK_CHANNEL_CODING_SET	    0x108
#define DP_SET_ANSI_8B10B		    (1 << 0)
#define DP_SET_ANSI_128B132B               (1 << 1)

#define DP_I2C_SPEED_CONTROL_STATUS	    0x109   /* DPI */
/* bitmask as for DP_I2C_SPEED_CAP */

#define DP_EDP_CONFIGURATION_SET            0x10a   /* XXX 1.2? */
#define DP_ALTERNATE_SCRAMBLER_RESET_ENABLE (1 << 0)
#define DP_FRAMING_CHANGE_ENABLE	    (1 << 1)
#define DP_PANEL_SELF_TEST_ENABLE	    (1 << 7)

#define DP_LINK_QUAL_LANE0_SET		    0x10b   /* DPCD >= 1.2 */
#define DP_LINK_QUAL_LANE1_SET		    0x10c
#define DP_LINK_QUAL_LANE2_SET		    0x10d
#define DP_LINK_QUAL_LANE3_SET		    0x10e
#define DP_LINK_QUAL_PATTERN_DISABLE	    0
#define DP_LINK_QUAL_PATTERN_D10_2	    1
#define DP_LINK_QUAL_PATTERN_ERROR_RATE    2
#define DP_LINK_QUAL_PATTERN_PRBS7	    3
#define DP_LINK_QUAL_PATTERN_80BIT_CUSTOM  4
#define DP_LINK_QUAL_PATTERN_HBR2_EYE      5
#define DP_LINK_QUAL_PATTERN_MASK	    7

#define DP_TRAINING_LANE0_1_SET2	    0x10f
#define DP_TRAINING_LANE2_3_SET2	    0x110
#define DP_LANE02_POST_CURSOR2_SET_MASK    (3 << 0)
#define DP_LANE02_MAX_POST_CURSOR2_REACHED (1 << 2)
#define DP_LANE13_POST_CURSOR2_SET_MASK    (3 << 4)
#define DP_LANE13_MAX_POST_CURSOR2_REACHED (1 << 6)

#define DP_MSTM_CTRL			    0x111   /* 1.2 */
#define DP_MST_EN			    (1 << 0)
#define DP_UP_REQ_EN			    (1 << 1)
#define DP_UPSTREAM_IS_SRC		    (1 << 2)

#define DP_AUDIO_DELAY0			    0x112   /* 1.2 */
#define DP_AUDIO_DELAY1			    0x113
#define DP_AUDIO_DELAY2			    0x114

#define DP_TRAIN_PRE_EMPHASIS_SHIFT	    3
#define DP_TRAIN_MAX_PRE_EMPHASIS_REACHED  (1 << 5)

#define DP_TRAINING_LANE0_1_SET2	    0x10f
#define DP_TRAINING_LANE2_3_SET2	    0x110
#define DP_LANE02_POST_CURSOR2_SET_MASK    (3 << 0)
#define DP_LANE02_MAX_POST_CURSOR2_REACHED (1 << 2)
#define DP_LANE13_POST_CURSOR2_SET_MASK    (3 << 4)
#define DP_LANE13_MAX_POST_CURSOR2_REACHED (1 << 6)

#define DP_LINK_RATE_SET		    0x115   /* eDP 1.4 */
#define DP_LINK_RATE_SET_SHIFT		    0
#define DP_LINK_RATE_SET_MASK		    (7 << 0)

#define DP_SINK_COUNT		0x200
#define DP_DEVICE_SERVICE_IRQ_VECTOR	    0x201
#define DP_REMOTE_CONTROL_COMMAND_PENDING  (1 << 0)
#define DP_AUTOMATED_TEST_REQUEST	    (1 << 1)
#define DP_CP_IRQ			    (1 << 2)
#define DP_MCCS_IRQ			    (1 << 3)
#define DP_DOWN_REP_MSG_RDY		    (1 << 4) /* 1.2 MST */
#define DP_UP_REQ_MSG_RDY		    (1 << 5) /* 1.2 MST */
#define DP_SINK_SPECIFIC_IRQ		    (1 << 6)


#define DP_LANE0_1_STATUS	0x202
#define DP_LANE2_3_STATUS	0x203
#define DP_LANE_CR_DONE		    (1 << 0)
#define DP_LANE_CHANNEL_EQ_DONE	    (1 << 1)
#define DP_LANE_SYMBOL_LOCKED		    (1 << 2)

#define DP_CHANNEL_EQ_BITS (DP_LANE_CR_DONE |		\
			    DP_LANE_CHANNEL_EQ_DONE |	\
			    DP_LANE_SYMBOL_LOCKED)

#define DP_LANE_ALIGN_STATUS_UPDATED                    0x204
#define DP_INTERLANE_ALIGN_DONE                        (1 << 0)
#define DP_128B132B_DPRX_EQ_INTERLANE_ALIGN_DONE       (1 << 2) /* 2.0 E11 */
#define DP_128B132B_DPRX_CDS_INTERLANE_ALIGN_DONE      (1 << 3) /* 2.0 E11 */
#define DP_128B132B_LT_FAILED                          (1 << 4) /* 2.0 E11 */
#define DP_DOWNSTREAM_PORT_STATUS_CHANGED              (1 << 6)
#define DP_LINK_STATUS_UPDATED                         (1 << 7)

#define DP_AUX_I2C_WRITE		0x0
#define DP_AUX_I2C_READ			0x1
#define DP_AUX_I2C_WRITE_STATUS_UPDATE	0x2
#define DP_AUX_I2C_MOT			0x4
#define DP_AUX_NATIVE_WRITE		0x8
#define DP_AUX_NATIVE_READ		0x9

#define DP_SUPPORTED_LINK_RATES 0x010 /* eDP 1.4 */
#define DP_MAX_SUPPORTED_RATES  8	  /* 16-bit little-endian */

#define DP_SET_POWER      0x600
#define DP_SET_POWER_D0   0x1
#define DP_SET_POWER_D3   0x2
#define DP_SET_POWER_MASK 0x3

#define DP_EDP_DPCD_REV			0x700    /* eDP 1.2 */
#define DP_EDP_11			    0x00
#define DP_EDP_12			    0x01
#define DP_EDP_13			    0x02
#define DP_EDP_14			    0x03

#define DP_REV_10 0x10
#define DP_REV_11 0x11
#define DP_REV_12 0x12
#define DP_REV_13 0x13
#define DP_REV_14 0x14

#define DP_ADJUST_REQUEST_LANE0_1	    0x206
#define DP_ADJUST_REQUEST_LANE2_3	    0x207
#define DP_ADJUST_VOLTAGE_SWING_LANE0_MASK  0x03
#define DP_ADJUST_VOLTAGE_SWING_LANE0_SHIFT 0
#define DP_ADJUST_PRE_EMPHASIS_LANE0_MASK   0x0c
#define DP_ADJUST_PRE_EMPHASIS_LANE0_SHIFT  2
#define DP_ADJUST_VOLTAGE_SWING_LANE1_MASK  0x30
#define DP_ADJUST_VOLTAGE_SWING_LANE1_SHIFT 4
#define DP_ADJUST_PRE_EMPHASIS_LANE1_MASK   0xc0
#define DP_ADJUST_PRE_EMPHASIS_LANE1_SHIFT  6


#define DP_PAYLOAD_TABLE_UPDATE_STATUS      0x2c0   /* 1.2 MST */
#define DP_PAYLOAD_TABLE_UPDATED           (1 << 0)
#define DP_PAYLOAD_ACT_HANDLED             (1 << 1)

#define DP_VC_PAYLOAD_ID_SLOT_1             0x2c1   /* 1.2 MST */
/* up to ID_SLOT_63 at 0x2ff */

#define DP_RECEIVER_CAP_SIZE		(0xf + 1)  // 16

#define DP_AUX_MAX_PAYLOAD_BYTES 16

#define DP_AUX_DEFAULT_BRIGHTNESS	(512/2)
#define DP_BRIGHTNESS_NITS_LSB		0x354
#define DP_BRIGHTNESS_MODE_PWM		(1)
#define DP_BRIGHTNESS_MODE_VESA		(2)
#define DP_BRIGHTNESS_MODE_HDR		(3)

#define DP_HPD_IRQ (0x1)
#define DP_HPD_IN  (0x2)
#define DP_HPD_OUT (0x4)

#define DP_RX_CAP_CHANGED                      (1 << 0) /* 1.2 */
#define DP_LINK_STATUS_CHANGED                 (1 << 1)
#define DP_STREAM_STATUS_CHANGED               (1 << 2)
#define DP_HDMI_LINK_STATUS_CHANGED            (1 << 3)
#define DP_CONNECTED_OFF_ENTRY_REQUESTED       (1 << 4)

#define DP_MODE(nm, t, c, hd, hss, hse, ht, hsk, vd, vss, vse, vt, vs, f) \
	.name = nm, .status = 0, .type = (t), .clock = (c), \
	.hdisplay = (hd), .hsync_start = (hss), .hsync_end = (hse), \
	.htotal = (ht), .hskew = (hsk), .vdisplay = (vd), \
	.vsync_start = (vss), .vsync_end = (vse), .vtotal = (vt), \
	.vscan = (vs), .flags = (f), \
	.base.type = DRM_MODE_OBJECT_MODE

struct dp_compliance_data {
	unsigned long edid;
	unsigned char video_pattern;
	unsigned short hdisplay, vdisplay;
	unsigned char bpc;
	unsigned char phy_pattern;
};

struct dp_compliance {
	unsigned long test_type;
	struct dp_compliance_data test_data;
	bool test_active;
	unsigned char test_link_rate;
	unsigned char test_lane_count;
};


#define DP_ADJUST_REQUEST_POST_CURSOR2      0x20c
#define DP_ADJUST_POST_CURSOR2_LANE0_MASK  0x03
#define DP_ADJUST_POST_CURSOR2_LANE0_SHIFT 0
#define DP_ADJUST_POST_CURSOR2_LANE1_MASK  0x0c
#define DP_ADJUST_POST_CURSOR2_LANE1_SHIFT 2
#define DP_ADJUST_POST_CURSOR2_LANE2_MASK  0x30
#define DP_ADJUST_POST_CURSOR2_LANE2_SHIFT 4
#define DP_ADJUST_POST_CURSOR2_LANE3_MASK  0xc0
#define DP_ADJUST_POST_CURSOR2_LANE3_SHIFT 6

typedef enum {
    BASE_DP0    = 0x190000,
    BASE_DP1    = 0x198000,
    BASE_DP_PHY = 0x198000
} dp_base_addr;


#define pll_prediv     (0)
#define pll_fbdiv      (1)
#define pll_postdiv    (2)
#define pll_clkdiv_16m (3)


struct aux_cfg {
	unsigned int aux_cmd;
	unsigned int dpcd_addr;
	unsigned int *wr_buff;
	unsigned int *rd_buff;
	unsigned int length;
	unsigned int rd_print;
	unsigned int read;
};

typedef struct {
	unsigned char dpcd_rev;
    unsigned char lane_rate;
    unsigned char lane_count;
	unsigned char enhance_mode;
    unsigned char phy_rate;
	unsigned char phy_lanes;
	unsigned char max_downspread;
    unsigned char swing[4];
    unsigned char preem[4];
    unsigned char pcursor[4];

    /* Displayport compliance testing */
	struct dp_compliance compliance;
    /* sink rates as reported by DP_MAX_LINK_RATE/DP_SUPPORTED_LINK_RATES */
	int max_sink_rates;
	int num_sink_rates;
	int sink_rates[DP_MAX_SUPPORTED_RATES];
	unsigned char cdr_delay;
	unsigned char eq_delay;

    unsigned char dpcd[DP_RECEIVER_CAP_SIZE];
	unsigned char sink_power_status;
    cea_parameter_t* cea_mode;
} dp_info;

struct drm_dp_aux_msg {
	unsigned int address;
	u8 request;
	u8 reply;
	void *buffer;
	size_t size;
};

void dpHandler(void);

/*
 * This is the main interrupt hook for HDMI engine.
 */
long hookDPInterrupt(
	dp_index index,
    void (*handler)(void)
);

/*
 * This function un-register HDMI Interrupt handler.
 */
long unhookDPInterrupt(
	dp_index index,
    void (*handler)(void)
);

void DP_Hpd_Interrupt_Enable(dp_index index, unsigned int enable);

int DP_Read_EDID(dp_index index, unsigned char *pEDIDBuffer, unsigned short *pBufferSize);
int DP_HPD_Detect(dp_index index);


int DP_Setmode(dp_index index, logicalMode_t *pLogicalMode , mode_parameter_t *pModeParam);


int DP_Init(dp_index index);
int DP_Enable(dp_index index);
int DP_Disable(dp_index index);
void DP_Set_Channel(dp_index index, disp_control_t dc);
void DP_Clear_Channel(dp_index index);
unsigned char DP_Get_Channel(dp_index index);

void DP_SSC_Enable(dp_index index, unsigned int downspread, unsigned int freq, unsigned int amp);
void DP_SSC_Disable(dp_index index);

void DP_Audio_CheckOverrun(dp_index index);

void DP_Audio_Mute(dp_index index);
void DP_Audio_UnMute(dp_index index);
void DP_Enable_Output(dp_index index);
void DP_Disable_Output(dp_index index);
void DP_Audio_Reset(dp_index index);

int Dp_HDP_Irq_Handle(dp_index index);
int DP_Check_Sink_Status(dp_index index);
long ddk770_DP_AdaptHWI2CInit(struct smi_connector *connector);
#endif  /* _DP_H_ */
