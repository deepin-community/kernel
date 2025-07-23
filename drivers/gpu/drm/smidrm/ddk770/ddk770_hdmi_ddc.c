#include "ddk770_os.h"
#include "ddk770_reg.h"
#include "ddk770_hardware.h"
#include "ddk770_power.h"
#include "ddk770_ddkdebug.h"
#include "ddk770_hwi2c.h"
#include "ddk770_hdmi_phy.h"
#include "ddk770_hdmi.h"
#include "ddk770_chip.h"
#include <linux/delay.h>
#include "ddk770_gpio.h"
#include "ddk770_hdmi_ddc.h"



/**
 * Find first (least significant) bit set
 * @param[in] data word to search
 * @return bit position or 32 if none is set
 */
static inline u32 first_bit_set(u32 data)
{
	u32 n = 0;

	if (data != 0) {
		for (n = 0; (data & 1) == 0; n++) {
			data >>= 1;
		}
	}
	return n;
}

/**
 * Set bit field
 * @param[in] data raw data
 * @param[in] mask bit field mask
 * @param[in] value new value
 * @return new raw data
 */
static inline u32 set(u32 data, u32 mask, u32 value)
{
	return (((value << first_bit_set(mask)) & mask) | (data & ~mask));
}

/**
 * calculate the fast sped high time counter - round up
 */
static u16 _scl_calc(u16 sfrClock, u16 sclMinTime)
{
	unsigned long tmp_scl_period = 0;
	if (((sfrClock * sclMinTime) % I2C_DIV_FACTOR) != 0) {
		tmp_scl_period = (unsigned long)((sfrClock * sclMinTime) + (I2C_DIV_FACTOR - ((sfrClock * sclMinTime) % I2C_DIV_FACTOR))) / I2C_DIV_FACTOR;
	}
	else {
		tmp_scl_period = (unsigned long)(sfrClock * sclMinTime) / I2C_DIV_FACTOR;
	}
	return (u16)(tmp_scl_period);
}


int _edid_checksum(u8 * edid)
{
	int i, checksum = 0;

	for(i = 0; i < EDID_LENGTH; i++)
		checksum += edid[i];

	return checksum % 256; //CEA-861 Spec
}




static void _fast_speed_high_clk_ctrl(hdmi_index index, u16 value)
{

	ddk770_HDMI_Write_Register(index, I2CM_FS_SCL_HCNT_1_ADDR, (u8) (value >> 8));
	ddk770_HDMI_Write_Register(index, I2CM_FS_SCL_HCNT_0_ADDR, (u8) (value >> 0));
}

static void _fast_speed_low_clk_ctrl(hdmi_index index, u16 value)
{

	ddk770_HDMI_Write_Register(index, I2CM_FS_SCL_LCNT_1_ADDR, (u8) (value >> 8));
	ddk770_HDMI_Write_Register(index, I2CM_FS_SCL_LCNT_0_ADDR, (u8) (value >> 0));
}

static void _standard_speed_high_clk_ctrl(hdmi_index index, u16 value)
{

	ddk770_HDMI_Write_Register(index, I2CM_SS_SCL_HCNT_1_ADDR, (u8) (value >> 8));
	ddk770_HDMI_Write_Register(index, I2CM_SS_SCL_HCNT_0_ADDR, (u8) (value >> 0));
}

static void _standard_speed_low_clk_ctrl(hdmi_index index, u16 value)
{

	ddk770_HDMI_Write_Register(index, I2CM_SS_SCL_LCNT_1_ADDR, (u8) (value >> 8));
	ddk770_HDMI_Write_Register(index, I2CM_SS_SCL_LCNT_0_ADDR, (u8) (value >> 0));
}




__attribute__((unused)) static void i2cddc_clk_config(hdmi_index index, u16 sfrClock, u16 ss_low_ckl, u16 ss_high_ckl, u16 fs_low_ckl, u16 fs_high_ckl)
{
	_standard_speed_low_clk_ctrl(index, _scl_calc(sfrClock, ss_low_ckl));
	_standard_speed_high_clk_ctrl(index, _scl_calc(sfrClock, ss_high_ckl));
	_fast_speed_low_clk_ctrl(index, _scl_calc(sfrClock, fs_low_ckl));
	_fast_speed_high_clk_ctrl(index, _scl_calc(sfrClock, fs_high_ckl));
}

int i2c_bus_clear(hdmi_index index)
{
	ddk770_HDMI_write_mask(index, I2CM_OPERATION, I2CM_OPERATION_BUSCLEAR_MASK, 1);
	
	return 0;
}

int i2c_reset(hdmi_index index)
{
	ddk770_HDMI_write_mask(index, I2CM_SOFTRSTZ, I2CM_SOFTRSTZ_I2C_SOFTRSTZ_MASK, 1);
	
	return 0;
}


static int ddci2c_write(hdmi_index index, u8 i2cAddr, u8 addr, u8 data)
{
	int timeout = I2CDDC_TIMEOUT;
	u32 status = 0;

	ddk770_HDMI_write_mask(index, I2CM_SLAVE, I2CM_SLAVE_SLAVEADDR_MASK, i2cAddr);
	ddk770_HDMI_Write_Register(index, I2CM_ADDRESS, addr);
	ddk770_HDMI_Write_Register(index, I2CM_DATAO, data);
	ddk770_HDMI_Write_Register(index, I2CM_OPERATION, I2CM_OPERATION_WRITE);
	do {
		usleep_range(5000, 5100);
		status = ddk770_HDMI_read_mask(index, IH_I2CM_STAT0, IH_I2CM_STAT0_I2CMASTERERROR_MASK |
							   IH_I2CM_STAT0_I2CMASTERDONE_MASK);
	} while (status == 0 && (timeout--));

	ddk770_HDMI_Write_Register(index, IH_I2CM_STAT0, status); //clear read status

	if(status & IH_I2CM_STAT0_I2CMASTERERROR_MASK){
		printk("error\n");
        return -1;
	}

	if(status & IH_I2CM_STAT0_I2CMASTERDONE_MASK){
		return 0;
	}
	
	return -1;

}

static int ddci2c_read8(hdmi_index index, u8 i2cAddr, u8 segment, u8 pointer, u32 addr, u8 * value)
{
	int timeout = I2CDDC_TIMEOUT;
	u32 status = 0;

	ddk770_HDMI_write_mask(index, I2CM_SLAVE, I2CM_SLAVE_SLAVEADDR_MASK, i2cAddr);
	ddk770_HDMI_Write_Register(index, I2CM_ADDRESS, addr);
	ddk770_HDMI_Write_Register(index, I2CM_SEGADDR, segment);
	ddk770_HDMI_Write_Register(index, I2CM_SEGPTR, pointer);

	if(pointer)
		ddk770_HDMI_Write_Register(index, I2CM_OPERATION, I2CM_OPERATION_READ_SEQ_EXT);
	else
		ddk770_HDMI_Write_Register(index, I2CM_OPERATION, I2CM_OPERATION_READ_SEQ);

	do {
	
		usleep_range(5000, 5100);
		status = ddk770_HDMI_read_mask(index, IH_I2CM_STAT0, IH_I2CM_STAT0_I2CMASTERERROR_MASK |
							   IH_I2CM_STAT0_I2CMASTERDONE_MASK);
	} while (status == 0 && (timeout--));

	ddk770_HDMI_Write_Register(index, IH_I2CM_STAT0, status); //clear read status

	if(status & IH_I2CM_STAT0_I2CMASTERERROR_MASK){
		printk("error\n");
		return -1;
	}


	if(status & IH_I2CM_STAT0_I2CMASTERDONE_MASK){
		int i = 0;
		while(i < 8){ //read 8 bytes
			value[i] = (u8) ddk770_HDMI_Read_Register(index, I2CM_READ_BUFF0 + i);
			i +=1;
		}
		return 0;
	}

	return -1;
}



static int ddci2c_read(hdmi_index index, u8 i2cAddr, u8 segment, u8 pointer, u32 addr,   u8 * value)
{
	int timeout = I2CDDC_TIMEOUT;
	u32 status = 0;

	ddk770_HDMI_write_mask(index, I2CM_SLAVE, I2CM_SLAVE_SLAVEADDR_MASK, i2cAddr);
	ddk770_HDMI_Write_Register(index, I2CM_ADDRESS, addr);
	ddk770_HDMI_Write_Register(index, I2CM_SEGADDR, segment);
	ddk770_HDMI_Write_Register(index, I2CM_SEGPTR, pointer);

	if(pointer)
		ddk770_HDMI_Write_Register(index, I2CM_OPERATION, I2CM_OPERATION_READ_EXT);
	else
		ddk770_HDMI_Write_Register(index, I2CM_OPERATION, I2CM_OPERATION_READ);

	do {
	
		usleep_range(5000, 5100);
		status = ddk770_HDMI_read_mask(index, IH_I2CM_STAT0, IH_I2CM_STAT0_I2CMASTERERROR_MASK |
							   IH_I2CM_STAT0_I2CMASTERDONE_MASK);
	} while (status == 0 && (timeout--));

	ddk770_HDMI_Write_Register(index, IH_I2CM_STAT0, status); //clear read status

	if(status & IH_I2CM_STAT0_I2CMASTERERROR_MASK){
		printk("error\n");
		return -1;
	}

	if(status & IH_I2CM_STAT0_I2CMASTERDONE_MASK){
		*value = (u8) ddk770_HDMI_Read_Register(index, I2CM_DATAI);
		return 0;
	}

	return -1;
}


int ddc_write(hdmi_index index, u8 i2cAddr, u8 addr, u8 len, u8 * data)
{
	int i, status = 0;

	for(i = 0; i < len; i++){
		int tries = 3;
		do {

			status = ddci2c_write(index, i2cAddr, addr, data[i]);
		} while (status && tries--);

		if(status) //Error after 3 failed writes
			return status;
	}
	return 0;
}


int ddc_read(hdmi_index index, u8 i2cAddr, u8 segment, u8 pointer, u8 addr, u32 len, u8 * data)
{
	int i, status = 0;

	for(i = 0; i < len;){
		int tries = 3;

		if ((len - i) >= 8){
			do {
				status = ddci2c_read8(index, i2cAddr, segment, pointer, addr + i,  &(data[i]));
			} while (status && tries--);

			if(status) //Error after 3 failed writes
				return status;
			
			i +=8;
		} else {

			do {
				status = ddci2c_read(index, i2cAddr, segment, pointer, addr + i,  &(data[i]));
			} while (status && tries--);

			if(status) //Error after 3 failed writes
				return status;
			
			i++;
		}
	
	}
	
	return 0;
}



static int scdc_read(hdmi_index index, u8 address, u8 size, u8 * data)
{
	if(ddc_read(index, SCDC_SLAVE_ADDRESS, 0,0 , address, size, data)){
		return -1;
	}
	return 0;
}

int scdc_write(hdmi_index index, u8 address, u8 size, u8 * data)
{
	if(ddc_write(index, SCDC_SLAVE_ADDRESS, address, size, data)){
		return -1;
	}
	return 0;
}


static int scdc_scrambling_enable_flag(hdmi_index index, u8 enable)
{
	u8 read_value = 0;
	if(scdc_read(index, SCDC_TMDS_CONFIG, 1 , &read_value)){
		return -1;
	}
	read_value = set(read_value, 0x1, enable ? 0x1 : 0x0);
	if(scdc_write(index, SCDC_TMDS_CONFIG, 1, &read_value)){
		return -1;
	}
	return 0;
}

int scdc_scrambling_status(hdmi_index index)
{
	u8 read_value = 0;
	if(scdc_read(index, SCDC_SCRAMBLER_STAT, 1, &read_value)){
		return 0;
	}
	return (read_value & 0x01);
}


int scdc_tmds_high_enable_flag(hdmi_index index, u8 enable)
{
	u8 read_value = 0;
	if(scdc_read(index, SCDC_TMDS_CONFIG, 1 , &read_value)){
		return -1;
	}
	read_value = set(read_value, 0x2, enable ? 0x1 : 0x0);
	if(scdc_write(index, SCDC_TMDS_CONFIG, 1, &read_value)){
		return -1;
	}
	return 0;
}

void scrambling_Enable(hdmi_index index, unsigned int enable)
{
	ddk770_HDMI_write_mask(index, FC_SCRAMBLER_CTRL, FC_SCRAMBLER_CTRL_SCRAMBLER_ON_MASK, enable);
}


static void mc_tmds_clock_reset(hdmi_index index, unsigned char bit)
{

	ddk770_HDMI_write_mask(index, MC_SWRSTZREQ, MC_SWRSTZREQ_TMDSSWRST_REQ_MASK, bit);
}


void scrambling(hdmi_index index, unsigned int enable){
	if (enable == 1) {
		scdc_scrambling_enable_flag(index, 1);
		usleep_range(100, 200);

		/* Start/stop HDCP keep-out window generation not needed because it's always on */
		/* TMDS software reset request */
		mc_tmds_clock_reset(index, 1);

		/* Enable/Disable Scrambling */
		scrambling_Enable(index, 1);
	} else {
		/* Enable/Disable Scrambling */
		scrambling_Enable(index, 0);
		scdc_scrambling_enable_flag(index, 0);

		/* TMDS software reset request */
		mc_tmds_clock_reset(index, 0);
	}
}




