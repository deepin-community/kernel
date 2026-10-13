#ifndef _LT8618_H_
#define _LT8618_H_

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define I2CADR  0x76
#define I2CHW
//#define lt8618_debug

#ifdef lt8618_debug
#define  smi_lt8618Msg(...) printk( __VA_ARGS__)
#else  
#define  smi_lt8618Msg(...)
#endif
//#define _HDCP
//#define _PATTERN_TEST
#define USE_DDRCLK  0
#define  _DEBUG_MODE
#define Audio_Input_Mode I2S_2CH
#define Video_Input_Mode    Input_RGB888
#define Video_Output_Mode  Output_RGB888
#define Tx_Out_Mode   HDMI_OUT

#define SUPPORTMODE 12

struct video_timing{
    u16 hfp;
    u16 hs;
    u16 hbp;
    u16 hact;
    u16 htotal;
    u16 vfp;
    u16 vs;
    u16 vbp;
    u16 vact;
    u16 vtotal;
	  u8  vic;
	  u8  vpol;
	  u8  hpol;
};

typedef struct _LT8618_SUPPORTMODE_
{
    u16 width;
    u16 hight;
    u32 vrefresh;
}LT8618_SUPPORTMODE;

enum LT8618SX_INPUTMODE_ENUM
{
    Input_RGB888,//yes
    Input_RGB_12BIT,
    Input_RGB565,
    Input_YCbCr444,//yes
    Input_YCbCr422_16BIT,//yes
    Input_YCbCr422_20BIT,//no
    Input_YCbCr422_24BIT,//no
    Input_BT1120_16BIT,//ok
    Input_BT1120_20BIT,//ok
    Input_BT1120_24BIT,//no
    Input_BT656_8BIT,//OK
    Input_BT656_10BIT,//no use
    Input_BT656_12BIT,//no use
	Input_BT601_8BIT//OK
};

enum VIDEO_OUTPUTMODE_ENUM
{
    Output_RGB888,
    Output_YCbCr444,
    Output_YCbCr422,
    Output_YCbCr422_16BIT,
    Output_YCbCr422_20BIT,
    Output_YCbCr422_24BIT
};

enum VideoFormat
{
    video_none,
    video_720x480i_60Hz_43=6,     
    video_720x480i_60Hz_169=7,    
	  video_1920x1080i_60Hz_169=5,  
	
    video_640x480_60Hz_vic1,      
    video_720x480_60Hz_vic3,     
	
	video_1280x720_24Hz_vic60,
    video_1280x720_25Hz_vic61,   
	video_1280x720_30Hz_vic62,
	video_1280x720_50Hz_vic19,
    video_1280x720_60Hz_vic4,   

    video_720x240P_60Hz_43=8,    
    video_720x240P_60Hz_169=9 ,  

    video_800x600_60Hz_vic,
    video_1280x1024_60Hz_vic,
    video_1680x1050_60Hz_vic,
    video_1400x900_60Hz_vic,
    video_1366x768_60Hz_vic,
    video_1280x960_60Hz_vic, 
    video_1280x800_60Hz_vic,
    video_1024x768_60Hz_vic,
	
	video_1920x1080_30Hz_vic34,
	video_1920x1080_50Hz_vic31,
    video_1920x1080_60Hz_vic16, 
    video_1920x1080I_60Hz_vic5,	
	
	video_3840x2160_24Hz_vic207,
	video_3840x2160_25Hz_vic206,
	video_3840x2160_30Hz_vic205
};

enum AUDIO_INPUTMODE_ENUM
{
    I2S_2CH,
	  I2S_8CH,
	  SPDIF
};

enum TX_OUT_MODE{
	DVI_OUT=0x00,
  HDMI_OUT=0x80
};

enum {
	_32KHz = 0,
	_44d1KHz,
	_48KHz,

	_88d2KHz,
	_96KHz,
	_176Khz,
	_196KHz
};

void LT8618SX_Output_Mode(char AudioOn);
void smi_lt8618Init(void);
int smi_lt8618SX_Task(unsigned long width, unsigned long height);


#endif