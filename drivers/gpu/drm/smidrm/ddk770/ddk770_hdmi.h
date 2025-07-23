#ifndef  _DDK770_HDMI_H_
#define  _DDK770_HDMI_H_

#include "ddk770_mode.h"
#include "ddk770_display.h"
#include "../hw_com.h"
#include "../smi_drv.h"

#define HDMI_BASE_OFFSET    0x20000

#define PHY_TIMEOUT         100

#define I2CM_OPERATION_READ		0x01
#define I2CM_OPERATION_READ_EXT		0x02
#define I2CM_OPERATION_READ_SEQ		0x04
#define I2CM_OPERATION_READ_SEQ_EXT     0x08
#define I2CM_OPERATION_WRITE		0x10

#define I2CDDC_TIMEOUT 10


typedef enum {
    COLOR_RGB,
    COLOR_YUV_420,
    COLOR_YUV_422,
    COLOR_YUV_444
} gColorFormat;

typedef enum {
    BASE_HDMI0    = 0x1a0000,
    BASE_HDMI1    = 0x1c0000,
    BASE_HDMI2    = 0x1e0000,
    BASE_HDMI_PHY = 0x198000,
    BASE_DC0      = 0x80000,
    BASE_DC1      = 0x88000,
    BASE_DC2      = 0x98000
} hdmi_base_addr;


void ddk770_i2c_reset_busclear(hdmi_index index);



void ddk770_HDMI_Write_Register(hdmi_index index, u32 offset, u32 value);
u32 ddk770_HDMI_Read_Register(hdmi_index index, u32 offset);
void ddk770_HDMI_Init(hdmi_index hdmi_index);

long ddk770_HDMI_Set_Mode(hdmi_index index, logicalMode_t *pLogicalMode, mode_parameter_t *pModeParam, bool isHDMI);

long ddk770_HDMI_HPD_Detect(hdmi_index index);
void ddk770_HDMI_set_SCDC(hdmi_index index, u8 *pEDIDBuffer);
long ddk770_HDMI_Read_EDID(hdmi_index index, u8 *pEDIDBuffer, u16* bufferSize);
int ddk770_HDMI_Read_EDID_Basic(hdmi_index index, u8 *pEDIDBuffer, u32 bufferSize, u16* pReadSize);

void ddk770_HDMI_Set_Channel(hdmi_index index, disp_control_t dc);
void ddk770_HDMI_Clear_Channel(hdmi_index index);
unsigned char ddk770_HDMI_Get_Channel(hdmi_index index);
void ddk770_HDMI_Audio_I2S_Configure(hdmi_index index);
void ddk770_HDMI_Enable_Output(hdmi_index index);
void ddk770_HDMI_Disable_Output (hdmi_index index);
void hdmiHandler(void);
void ddk770_HDMI_Clear_Intr_State(hdmi_index index);
void irq_mask_all(hdmi_index index);

void ddk770_HDMI_Audio_Unmute(hdmi_index index);
void ddk770_HDMI_Audio_Mute(hdmi_index index);

/*
 * This is the main interrupt hook for HDMI engine.
 */
long ddk770_hookHDMIInterrupt(
	hdmi_index index,
    void (*handler)(void)
);


/*
 * This function un-register HDMI Interrupt handler.
 */
long ddk770_unhookHDMIInterrupt(
	hdmi_index index,
    void (*handler)(void)
);

long ddk770_HDMI_Intr_Mute(hdmi_index index,u32 mute);

u32 ddk770_HDMI_read_mask(hdmi_index index, u32 addr, u32 mask);
void ddk770_HDMI_write_mask(hdmi_index index, u32 addr, u32 mask, u32 data);



void ddk770_HDMI_interrupt_enable(hdmi_index index, unsigned int enable);



int ddk770_HDMI_Standby(hdmi_index index);
int hdmiISR(hdmi_index index);
long ddk770_HDMI_AdaptHWI2CInit(struct smi_connector *connector);


#endif  /* _HDMI_H_ */
