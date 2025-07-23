#ifndef  _DDK770_HDMI_PHY_H_
#define  _DDK770_HDMI_PHY_H_

#include "ddk770_hdmi.h"

#define PHY_I2C_SLAVE_ADDR   	0x69

#define OPMODE_PLLCFG 0x06 //Mode of Operation and PLL  Dividers Control Register
#define CKSYMTXCTRL   0x09 //Clock Symbol and Transmitter Control Register
#define PLLCURRCTRL   0x10 //PLL Current Control Register
#define VLEVCTRL      0x0E //Voltage Level Control Register
#define PLLGMPCTRL    0x15 //PLL Gmp Control Register
#define TXTERM        0x19 //Transmission Termination Register

#define LT_1_485GBPS_TXTERM			0x0004
#define LT_1_485GBPS_VLEVCTRL		0x0232
#define LT_1_485GBPS_CKSYMTXCTRL	0x8009

#define LT_2_22GBPS_TXTERM			0x0004
#define LT_2_22GBPS_VLEVCTRL		0x0230
#define LT_2_22GBPS_CKSYMTXCTRL		0x8009

#define LT_2_97GBPS_TXTERM			0x0004
#define LT_2_97GBPS_VLEVCTRL		0x0273
#define LT_2_97GBPS_CKSYMTXCTRL		0x8009

#define LT_3_40GBPS_TXTERM			0x0004
#define LT_3_40GBPS_VLEVCTRL		0x0273
#define LT_3_40GBPS_CKSYMTXCTRL		0x8029

#define GT_3_40GBPS_TXTERM          0x0000
#define GT_3_40GBPS_VLEVCTRL        0x00C6
#define GT_3_40GBPS_CKSYMTXCTRL     0x8039

#define LT_1_485GBPS LT_1_485GBPS_TXTERM, LT_1_485GBPS_VLEVCTRL, LT_1_485GBPS_CKSYMTXCTRL
#define LT_2_22GBPS LT_2_22GBPS_TXTERM, LT_2_22GBPS_VLEVCTRL, LT_2_22GBPS_CKSYMTXCTRL
#define LT_2_97GBPS LT_2_97GBPS_TXTERM, LT_2_97GBPS_VLEVCTRL, LT_2_97GBPS_CKSYMTXCTRL
#define LT_3_40GBPS LT_3_40GBPS_TXTERM, LT_3_40GBPS_VLEVCTRL, LT_3_40GBPS_CKSYMTXCTRL
#define GT_3_40GBPS GT_3_40GBPS_TXTERM, GT_3_40GBPS_VLEVCTRL, GT_3_40GBPS_CKSYMTXCTRL

typedef enum {
	COLOR_DEPTH_INVALID = 0,
	COLOR_DEPTH_8 = 8,
	COLOR_DEPTH_10 = 10,
	COLOR_DEPTH_12 = 12,
	COLOR_DEPTH_16 = 16
} color_depth_t;

typedef enum {
	PIXEL_REPETITION_OFF = 0,
	PIXEL_REPETITION_1 = 1,
	PIXEL_REPETITION_2 = 2,
	PIXEL_REPETITION_3 = 3,
	PIXEL_REPETITION_4 = 4,
	PIXEL_REPETITION_5 = 5,
	PIXEL_REPETITION_6 = 6,
	PIXEL_REPETITION_7 = 7,
	PIXEL_REPETITION_8 = 8,
	PIXEL_REPETITION_9 = 9,
	PIXEL_REPETITION_10 = 10
} pixel_repetition_t;

typedef enum {
	HDMI_14 = 1,
	HDMI_20,
	MHL_24 ,
	MHL_PACKEDPIXEL
} operation_mode_t;

typedef enum {
	ENC_UNDEFINED = -1,
	RGB = 0,
	YCC444,
	YCC422,
	YCC420
} encoding_t;

typedef enum {
	UNDEFINED_COLORIMETRY = 0xFF,
	ITU601 = 1,
	ITU709,
	EXTENDED_COLORIMETRY
} colorimetry_t;

typedef enum {
	UNDEFINED_EXTCOLOR = 0xFF,
	XV_YCC601 = 0,
	XV_YCC709,
	S_YCC601,
	ADOBE_YCC601,
	ADOBE_RGB
} ext_colorimetry_t;

typedef enum {
	UNDEFINED_FORMAT = 0xFF,
	HDMI_NORMAL_FORMAT = 0,
	HDMI_EXT_RES_FORMAT = 1,
	HDMI_3D_FORMAT = 2,
} hdmi_video_format_t;

typedef enum {
	UNDEFINED_3D = 0xFF,
	FRAME_PACKING_3D = 0,
	TOP_AND_BOTTOM_3D = 6,
	SIDE_BY_SIDE_3D = 8
} hdmi_3d_structure_t;

typedef enum {
	UNDEFINED_EXTDATA_3D = 0,
} hdmi_3d_extdata_t;

typedef enum {
	UNDEFINED_HDMI_VIC = 0xFF,
} hdmi_vic_t;


struct phy_config{
	unsigned int 			clock;
	pixel_repetition_t 	pixel;
	color_depth_t      	color;
	operation_mode_t 	opmode;
	unsigned short		 	oppllcfg;
	unsigned short			pllcurrctrl;
	unsigned short			pllgmpctrl;
	unsigned short                 	txterm;
	unsigned short                 	vlevctrl;
	unsigned short                 	cksymtxctrl;
};

int ddk770_HDMI_PHY_Set_Mode(hdmi_index index, u32 pClk);
int phy_initialize(hdmi_index index);
void ddk770_HDMI_phy_interrupt_unmask(hdmi_index index, u8 mask);
void ddk770_HDMI_phy_interrupt_mask(hdmi_index index, u8 mask);
int ddk770_HDMI_phy_i2c_write(hdmi_index index, u8 addr, u16 data);
int ddk770_HDMI_phy_i2c_read(hdmi_index index, u8 addr, u16 * value);
void ddk770_HDMI_phy_i2c_slave_address(hdmi_index index, u8 value);
int phy_standby(hdmi_index index);
void irq_hpd_sense_enable(hdmi_index index);
int decode_is_phy(u32 decode);
int decode_is_phy_hpd(u32 decode);
u32 read_interrupt_decode(hdmi_index index);
int phy_hot_plug_detected(hdmi_index index);
u8 phy_rx_sense_state(hdmi_index index);
u8 phy_hot_plug_state(hdmi_index index);
u8 u32_is_equal(u32 a, u32 b);


#endif
