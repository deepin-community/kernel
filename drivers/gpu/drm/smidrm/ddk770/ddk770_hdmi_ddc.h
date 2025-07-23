#ifndef  _DDK770_HDMI_DDC_H_
#define  _DDK770_HDMI_DDC_H_

#include "ddk770_hdmi.h"


#define EDID_I2C_ADDR  		0x50
#define EDID_I2C_SEGMENT_ADDR  	0x30
#define DDC_CI_ADDR		0x37
#define EDID_LENGTH 128


#define I2C_DIV_FACTOR	 100000
#define I2C_MIN_FS_SCL_HIGH_TIME   61 //63 //75
#define I2C_MIN_FS_SCL_LOW_TIME    132 //137 //163
#define I2C_MIN_SS_SCL_HIGH_TIME   4592 //4737 //5625
#define I2C_MIN_SS_SCL_LOW_TIME    5102 //5263 //6250

/* 00=16:10, 01=4:3, 10=5:4, 11=16:9 */
#define EDID_TIMING_ASPECT_SHIFT 6
#define EDID_TIMING_ASPECT_MASK  (0x3 << EDID_TIMING_ASPECT_SHIFT)

/* need to add 60 */
#define EDID_TIMING_VFREQ_SHIFT  0
#define EDID_TIMING_VFREQ_MASK   (0x3f << EDID_TIMING_VFREQ_SHIFT)

#define DRM_EDID_PT_HSYNC_POSITIVE (1 << 1)
#define DRM_EDID_PT_VSYNC_POSITIVE (1 << 2)
#define DRM_EDID_PT_SEPARATE_SYNC  (3 << 3)
#define DRM_EDID_PT_STEREO         (1 << 5)
#define DRM_EDID_PT_INTERLACED     (1 << 7)

#define EDID_DETAIL_EST_TIMINGS 0xf7
#define EDID_DETAIL_CVT_3BYTE 0xf8
#define EDID_DETAIL_COLOR_MGMT_DATA 0xf9
#define EDID_DETAIL_STD_MODES 0xfa
#define EDID_DETAIL_MONITOR_CPDATA 0xfb
#define EDID_DETAIL_MONITOR_NAME 0xfc
#define EDID_DETAIL_MONITOR_RANGE 0xfd
#define EDID_DETAIL_MONITOR_STRING 0xfe
#define EDID_DETAIL_MONITOR_SERIAL 0xff

#define EDID_PRODUCT_ID(e) ((e)->prod_code[0] | ((e)->prod_code[1] << 8))


int _edid_checksum(u8 * edid);
void scrambling(hdmi_index index, unsigned int enable);
void scrambling_Enable(hdmi_index index, unsigned int enable);
int scdc_tmds_high_enable_flag(hdmi_index index, u8 enable);
int scdc_scrambling_status(hdmi_index index);
int ddc_read(hdmi_index index, u8 i2cAddr, u8 segment, u8 pointer, u8 addr, u32 len, u8 * data);
int scdc_write(hdmi_index index, u8 address, u8 size, u8 * data);
int i2c_bus_clear(hdmi_index index);
int i2c_reset(hdmi_index index);
int ddc_write(hdmi_index index, u8 i2cAddr, u8 addr, u8 len, u8 * data);



#endif

