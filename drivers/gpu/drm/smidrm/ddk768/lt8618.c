#include "ddk768_swi2c.h"
#include "ddk768_hwi2c.h"
#include "ddk768_timer.h"
#include "lt8618.h"
#include <linux/kernel.h>
#include<linux/delay.h>
/*note phease_offset should be setted according to TTL timing.*/

/*************************************
   Resolution			HDMI_VIC
   --------------------------------------
   640x480		1
   720x480P 60Hz		2
   720x480i 60Hz		6

   720x576P 50Hz		17
   720x576i 50Hz		21

   1280x720P 24Hz		60
   1280x720P 25Hz		61
   1280x720P 30Hz		62
   1280x720P 50Hz		19
   1280x720P 60Hz		4

   1920x1080P 24Hz	32
   1920x1080P 25Hz	33
   1920x1080P 30Hz	34

   1920x1080i 50Hz		20
   1920x1080i 60Hz		5

   1920x1080P 50Hz	31
   1920x1080P 60Hz	16

   3840x2160P 24Hz	207
   3840x2160P 25Hz	206
   3840x2160P 30Hz	205

   Other resolution	0(default)

 **************************************/

enum VideoFormat Video_Format;

u16 hfp,hs_width,hbp,h_act,h_tal,vfp,vs_width,vbp,v_act,v_tal,v_pol,h_pol;
u8 HDMI_VIC=0x00;
u8 HDMI_Y=0x00;
u8 vid_chk_flag=1;
u8 intb_flag=0;
u8 phease_offset=0;
struct video_timing video_bt={0};

enum{
_16bit = 1,
_20bit,
_24_32bit
};

#define Sampling_rate	_48KHz
#define Sampling_Size	_24_32bit

// hfp, hs, hbp,hact,htotal,vfp, vs, vbp,vact,vtotal,vic,vpol,hpol,
const struct video_timing video_720x480_60Hz     = {16, 62,  60, 720,   858,  9,  6,  30, 480,   525};//ok
const struct video_timing video_800x600_60Hz     = {16, 62,  60, 800,   1056,  9,  6,  30, 600,   628};//ok
const struct video_timing video_1280x720_60Hz    = {110,40, 220,1280,  1650,  6,  5,  19, 720,   750};//ok
const struct video_timing video_1280x720_30Hz   = {736, 40, 1244,1280,  3300, 5,  5,  20, 720,   750};//no src
const struct video_timing video_1920x1080_60Hz   = {88,44, 148,1920,  2200,  4,  5, 36, 1080,1125};//ok
const struct video_timing video_1280x1024_60Hz   = {48,112, 248,1280,  1688,  1,  3, 38, 1024, 1066};//ok

const struct video_timing video_1680x1050_60Hz   = { 104, 176, 280, 1680,   2240,   3,  6,  30,  1050,  1089};//
const struct video_timing video_1440x900_60Hz    = { 80,  152, 232, 1440,   1904,   3,  6,  25,  900,    934};//ok
const struct video_timing video_1366x768_60Hz    = { 70,  143, 213, 1366,   1792,   3,  3,  24,  768,    798};//ok
const struct video_timing video_1280x960_60Hz    = { 96,  112, 312, 1280,   1800,   1,  3,  36,  960,   1000};//ok
const struct video_timing video_1280x800_60Hz    = { 72,  128, 200, 1280,   1680,   3,  6,  22,  800,    831};//ok
const struct video_timing video_3840x2160_30Hz   = {176,88, 296,3840,  4400,  8,  10, 72, 2160, 2250};
const struct video_timing video_1024x768_60Hz   = {24,136, 160,1024, 1344,  3,  6, 29, 768, 806};

LT8618_SUPPORTMODE lt8618_SupportMode[SUPPORTMODE] = {
	{720,480,60},{800,600,60},{1280,720,60},{1280,720,30},
	{1920,1080,60},{1280,1024,60},{1680,1050,60},{1440,900,60},
	{1366,768,60},{1280,960,60},{1280,800,60},{1024,768,60}
};

 u16 Sample_Freq[] =
{
	0x30,   // 32K
	0x00,   // 44.1K
	0x20,   // 48K
	0x80,   // 88.2K
	0xa0,   // 96K
	0xc0,   // 176K
	0xe0    //  196K
};


u16 IIS_N[] =
{
	4096,   // 32K
	6272,   // 44.1K
	6144,   // 48K
	12544,  // 88.2K
	12288,  // 96K
	25088,  // 176K
	24576   // 196K
};


int  lt8618_WriteI2C_Byte( u8 RegAddr, u8 data )
{
	int ret;
#ifdef I2CHW
	ret = ddk768_hwI2CWriteReg(0, I2CADR, RegAddr, data);
#else
	ret = ddk768_swI2CWriteReg( I2CADR, RegAddr, data );
#endif
	
	return ret;
}

u8 lt8618_ReadI2C_Byte( u8 RegAddr )
{
	u8 data = 0;
#ifdef I2CHW
	data = ddk768_hwI2CReadReg(0, I2CADR, RegAddr);
#else	
	data = ddk768_swI2CReadReg( I2CADR, RegAddr);
#endif
	return data;
}

void LT8618SX_RST_PD_Init(void)
{
	lt8618_WriteI2C_Byte(0xff,0x80);
	lt8618_WriteI2C_Byte(0x11,0x00); //reset MIPI Rx logic
	lt8618_WriteI2C_Byte(0x13,0xf1);
	lt8618_WriteI2C_Byte(0x13,0xf9); //reset TTL video process
}

void LT8618SX_TTL_Input_Analog(void)
{
	
	//TTL mode
	lt8618_WriteI2C_Byte(0xff,0x81);
	lt8618_WriteI2C_Byte(0x02,0x66);
	lt8618_WriteI2C_Byte(0x0a,0x06);//0x06
	lt8618_WriteI2C_Byte(0x15,0x06);
	lt8618_WriteI2C_Byte(0x4e,0x00);//
	
	lt8618_WriteI2C_Byte( 0xff,0x82);// ring
	lt8618_WriteI2C_Byte( 0x1b,0x77);
	lt8618_WriteI2C_Byte( 0x1c,0xec); 

}

void LT8618SX_TTL_Input_Digital(void)
{
	//TTL mode
	if(Video_Input_Mode == Input_RGB888)
	{
			smi_lt8618Msg("Video_Input_Mode=Input_RGB888\n");
			lt8618_WriteI2C_Byte(0xff,0x82);
			lt8618_WriteI2C_Byte(0x45,0x70); //RGB channel swap
			if(USE_DDRCLK == 1)
			{
			lt8618_WriteI2C_Byte(0x4f,0xc0); //0x80;  0xc0: invert dclk 
			}
			else
			{
			lt8618_WriteI2C_Byte(0x4f,0x40); //0x00;  0x40: invert dclk 
			}
			lt8618_WriteI2C_Byte(0x50,0x00);
			lt8618_WriteI2C_Byte(0x51,0x00);
			
			#ifdef Only_De_Mode
			lt8618_WriteI2C_Byte(0xff,0x80);
			lt8618_WriteI2C_Byte(0x0a,0x90);
			
			lt8618_WriteI2C_Byte(0xff,0x82);
			lt8618_WriteI2C_Byte(0x47,0x47);
			#endif
	}
	else if(Video_Input_Mode == Input_RGB_12BIT)
	{
			smi_lt8618Msg("Video_Input_Mode=Input_RGB_12BIT\n");
		    lt8618_WriteI2C_Byte(0xff,0x80);
			lt8618_WriteI2C_Byte(0x0a,0x80);
		
			lt8618_WriteI2C_Byte(0xff,0x82);
			lt8618_WriteI2C_Byte(0x45,0x70); //RGB channel swap
			
			lt8618_WriteI2C_Byte(0x4f,0x40); //0x00;  0x40: invert dclk 
			
			lt8618_WriteI2C_Byte(0x50,0x00);
			lt8618_WriteI2C_Byte(0x51,0x30);
			lt8618_WriteI2C_Byte(0x40,0x00);
			lt8618_WriteI2C_Byte(0x41,0xcd);
	}
	else if(Video_Input_Mode == Input_YCbCr444)
	{
			smi_lt8618Msg("Video_Input_Mode=Input_YCbCr444\n");
			lt8618_WriteI2C_Byte(0xff,0x82);
			lt8618_WriteI2C_Byte(0x45,0x70); //RGB channel swap
			lt8618_WriteI2C_Byte(0x4f,0x40); //0x00;  0x40: dclk 
	}
	else if(Video_Input_Mode==Input_YCbCr422_16BIT)
	{
			smi_lt8618Msg("Video_Input_Mode=Input_YCbCr422_16BIT\n");
			lt8618_WriteI2C_Byte(0xff,0x82);
			lt8618_WriteI2C_Byte(0x45,0x00); //RGB channel swap
			if(USE_DDRCLK == 1)
			{
			lt8618_WriteI2C_Byte(0x4f,0x40); //0x80;  0xc0: invert dclk 
			}
			else
			{
			lt8618_WriteI2C_Byte(0x4f,0x00); //0x00;  0x40: invert dclk 
			}
	}
	
	else if(Video_Input_Mode==Input_BT1120_16BIT)
	{
			smi_lt8618Msg("Video_Input_Mode=Input_BT1120_16BIT\n");
			lt8618_WriteI2C_Byte(0xff,0x82);
			//lt8618_WriteI2C_Byte(0x45,0x30); // D0 ~ D7 Y ; D8 ~ D15 C
			//lt8618_WriteI2C_Byte(0x45,0x70); // D8 ~ D15 Y ; D16 ~ D23 C
			lt8618_WriteI2C_Byte(0x45,0x00); // D0 ~ D7 C ; D8 ~ D15 Y
			//lt8618_WriteI2C_Byte(0x45,0x60); // D8 ~ D15 C ; D16 ~ D23 Y
			if(USE_DDRCLK == 1)
			{
			lt8618_WriteI2C_Byte(0x4f,0x40); //0x80;  0xc0: invert dclk 
			}
			else
			{
			lt8618_WriteI2C_Byte(0x4f,0xc0); //0x00;  0x40: invert dclk 
			}
			lt8618_WriteI2C_Byte(0x48,0x1c); //0x1c  0x08 Embedded sync mode input enable.
			lt8618_WriteI2C_Byte(0x51,0x42); //0x02 0x43 0x34 embedded DE mode input edable.	
		    lt8618_WriteI2C_Byte(0x47,0x07);
	}
	else if(Video_Input_Mode==Input_BT1120_20BIT)
	{
			smi_lt8618Msg("Video_Input_Mode=Input_BT1120_20BIT\n");
			lt8618_WriteI2C_Byte(0xff,0x82);
			lt8618_WriteI2C_Byte(0x45,0x00); 

			lt8618_WriteI2C_Byte(0x4f,0x00); //0x00;  0x40: invert dclk 
			
		    lt8618_WriteI2C_Byte(0x46,0x0e); 
		    lt8618_WriteI2C_Byte(0x47,0x07);
			lt8618_WriteI2C_Byte(0x48,0x1d); 
			lt8618_WriteI2C_Byte(0x51,0x42); 	
	}
	else if(Video_Input_Mode==Input_BT656_8BIT)
	{
			smi_lt8618Msg("Video_Input_Mode=Input_BT656_8BIT\n");
			lt8618_WriteI2C_Byte(0xff,0x82);
			lt8618_WriteI2C_Byte(0x45,0x00); //RGB channel swap
			if(USE_DDRCLK == 1)
			{
			lt8618_WriteI2C_Byte(0x4f,0xc0); //0x80;  0xc0: dclk
			lt8618_WriteI2C_Byte(0x48,0x48);//0x50 0x5c 0x40 0x48 
			}
			else
			{
			lt8618_WriteI2C_Byte(0x4f,0xc0); //0x00;  0x40: dclk 
			lt8618_WriteI2C_Byte(0x48,0x5c);//0x48
			}
			lt8618_WriteI2C_Byte(0x51,0x42); 
			lt8618_WriteI2C_Byte(0x47,0x07); 
	}		
	else if(Video_Input_Mode==Input_BT601_8BIT)
	{
			smi_lt8618Msg("Video_Input_Mode=Input_BT601_8BIT\n");
		    lt8618_WriteI2C_Byte(0xff,0x81);
			lt8618_WriteI2C_Byte(0x0a,0x90);
		
		    lt8618_WriteI2C_Byte(0xff,0x81);
			lt8618_WriteI2C_Byte(0x4e,0x02);
		
			lt8618_WriteI2C_Byte(0xff,0x82);
			lt8618_WriteI2C_Byte(0x45,0x00); //RGB channel swap
			if(USE_DDRCLK == 1)
			{
			lt8618_WriteI2C_Byte(0x4f,0xc0); //0x80;  0xc0: dclk
			lt8618_WriteI2C_Byte(0x48,0x5c);//0x50 0x5c 0x40 0x48 
			}
			else
			{
			lt8618_WriteI2C_Byte(0x4f,0xc0); //0x00;  0x40: dclk 
			lt8618_WriteI2C_Byte(0x48,0x40);
			}
			lt8618_WriteI2C_Byte(0x51,0x00); 
			lt8618_WriteI2C_Byte(0x47,0x87); 
	}		
}

u32 LT8618SX_CLK_Det(void)
{
  u32 dclk_;
  lt8618_WriteI2C_Byte(0xff,0x82);
  lt8618_WriteI2C_Byte(0x17,0x80);
  mdelay(500);
  dclk_=(((lt8618_ReadI2C_Byte(0x1d))&0x0f)<<8)+lt8618_ReadI2C_Byte(0x1e);
  dclk_=(dclk_<<8)+lt8618_ReadI2C_Byte(0x1f);
  smi_lt8618Msg("LT8618SX ad ttl dclk = %d\n", dclk_); 
  return dclk_;
}

int LT8618SX_PLL_Version_U2(void)
{
	  u8 read_val;
	  u8 j;
	  u8 cali_done;
	  u8 cali_val;
	  u8 lock;
	  u8 dclk;
	
	  dclk=LT8618SX_CLK_Det();
	
    if(Video_Input_Mode==Input_RGB888 || Video_Input_Mode==Input_YCbCr444||
			Video_Input_Mode==Input_YCbCr422_16BIT||Video_Input_Mode==Input_BT1120_16BIT||Video_Input_Mode==Input_BT1120_20BIT)
    {
	          lt8618_WriteI2C_Byte( 0xff, 0x81 );
			  lt8618_WriteI2C_Byte( 0x23, 0x40 );
			  lt8618_WriteI2C_Byte( 0x24, 0x62 );               //icp set
			  lt8618_WriteI2C_Byte( 0x26, 0x55 );
	      
	     if(dclk<=25000)
			 {
				lt8618_WriteI2C_Byte( 0x25, 0x00 );
				//lt8618_WriteI2C_Byte( 0x2c, 0xA8 );
				lt8618_WriteI2C_Byte( 0x2c, 0x94 );
				lt8618_WriteI2C_Byte( 0x2d, 0xaa );
			 }
	     else if((dclk>25000)&&(dclk<=50000))
			 {
				lt8618_WriteI2C_Byte( 0x25, 0x00 );
				lt8618_WriteI2C_Byte( 0x2d, 0xaa );
				lt8618_WriteI2C_Byte( 0x2c, 0x94 );
			 }
	     else if((dclk>50000)&&(dclk<=100000))
			 {
				lt8618_WriteI2C_Byte( 0x25, 0x01 );
				lt8618_WriteI2C_Byte( 0x2d, 0x99 );
				lt8618_WriteI2C_Byte( 0x2c, 0x94 );
				
				// smi_lt8618Msg("\r\n50~100m");
			 }
			 
			 else //if(dclk>100000)
			 {
				lt8618_WriteI2C_Byte( 0x25, 0x03 );
				lt8618_WriteI2C_Byte( 0x2d, 0x88 );
				lt8618_WriteI2C_Byte( 0x2c, 0x94 );
			 }
			 
			 if( USE_DDRCLK )
			 {
				 read_val=lt8618_ReadI2C_Byte(0x2c) &0x7f;
				 read_val=read_val*2|0x80;
				 lt8618_WriteI2C_Byte( 0x2c, read_val );
				 
				 lt8618_WriteI2C_Byte( 0x4d, 0x05 );
				 lt8618_WriteI2C_Byte( 0x27, 0x66 );                                               //0x60 //ddr 0x66
				 lt8618_WriteI2C_Byte( 0x28, 0x88 ); 
                 smi_lt8618Msg("LT8618SX PLL DDR\n" );				 
			 }
			else
			{
				lt8618_WriteI2C_Byte( 0x4d, 0x01 );
				lt8618_WriteI2C_Byte( 0x27, 0x60 ); //0x06                                              //0x60 //ddr 0x66
				lt8618_WriteI2C_Byte( 0x28, 0x88 );                                               // 0x88
				smi_lt8618Msg("LT8618SX PLL SDR \n" );
			}
    }
	
    else if(Video_Input_Mode==Input_BT656_8BIT||Video_Input_Mode==Input_BT656_10BIT||Video_Input_Mode==Input_BT656_12BIT)
				{
					;
				}
			  
  	    // as long as changing the resolution or changing the input clock,	You need to configure the following registers.
				lt8618_WriteI2C_Byte( 0xff, 0x81 );
				read_val=lt8618_ReadI2C_Byte(0x2b);
				lt8618_WriteI2C_Byte( 0x2b, read_val&0xfd );// sw_en_txpll_cal_en
				read_val=lt8618_ReadI2C_Byte(0x2e);
				lt8618_WriteI2C_Byte( 0x2e, read_val&0xfe );//sw_en_txpll_iband_set
				
				lt8618_WriteI2C_Byte( 0xff, 0x82 );
				lt8618_WriteI2C_Byte( 0xde, 0x00 );
				lt8618_WriteI2C_Byte( 0xde, 0xc0 );
				
                lt8618_WriteI2C_Byte( 0xff, 0x80 );
				lt8618_WriteI2C_Byte( 0x16, 0xf1 );
				lt8618_WriteI2C_Byte( 0x18, 0xdc );//txpll _sw_rst_n
				lt8618_WriteI2C_Byte( 0x18, 0xfc );
				lt8618_WriteI2C_Byte( 0x16, 0xf3 );	
					  for(j=0;j<0x05;j++)
				    {
						mdelay(10);
						lt8618_WriteI2C_Byte(0xff,0x80);	
			            lt8618_WriteI2C_Byte(0x16,0xe3); /* pll lock logic reset */
			            lt8618_WriteI2C_Byte(0x16,0xf3);
							
						lt8618_WriteI2C_Byte( 0xff, 0x82 );
						lock=0x80&lt8618_ReadI2C_Byte(0x15);	
						cali_val=lt8618_ReadI2C_Byte(0xea);	
						cali_done=0x80&lt8618_ReadI2C_Byte(0xeb);
						if(lock&&cali_done&&(cali_val!=0xff))
						{	
#ifdef _DEBUG_MODE
						lt8618_WriteI2C_Byte( 0xff, 0x82 );
						smi_lt8618Msg("0x82ea=0x%x\n",lt8618_ReadI2C_Byte(0xea));	
						smi_lt8618Msg("0x82eb=0x%x\n",lt8618_ReadI2C_Byte(0xeb));	
						smi_lt8618Msg("0x82ec=0x%x\n",lt8618_ReadI2C_Byte(0xec));	
						smi_lt8618Msg("0x82ed=0x%x\n",lt8618_ReadI2C_Byte(0xed));	
						smi_lt8618Msg("0x82ee=0x%x\n",lt8618_ReadI2C_Byte(0xee));	
						smi_lt8618Msg("0x82ef=0x%x\n",lt8618_ReadI2C_Byte(0xef));	
#endif										
						smi_lt8618Msg("TXPLL Lock\n");
						return 1;
						}
						else
						{
						lt8618_WriteI2C_Byte( 0xff, 0x80 );
						lt8618_WriteI2C_Byte( 0x16, 0xf1 );
						lt8618_WriteI2C_Byte( 0x18, 0xdc );//txpll _sw_rst_n
						lt8618_WriteI2C_Byte( 0x18, 0xfc );
						lt8618_WriteI2C_Byte( 0x16, 0xf3 );	
						smi_lt8618Msg("TXPLL Reset\n");
						}
					  }
				smi_lt8618Msg("TXPLL Unlock\n");
				return 0;
}

int  LT8618SX_PLL_Version_U3(void)
{
	  u8 read_val;
	  u8 j;
	  u8 cali_done;
	  u8 cali_val;
	  u8 lock;
	  u32 dclk;
	
	  dclk=LT8618SX_CLK_Det();
	
	  if(USE_DDRCLK == 1)
				{
				 dclk=dclk*2;
				}
	
    if(Video_Input_Mode==Input_RGB_12BIT||Video_Input_Mode==Input_RGB888 || Video_Input_Mode==Input_YCbCr444||
			Video_Input_Mode==Input_YCbCr422_16BIT||Video_Input_Mode==Input_BT1120_16BIT||Video_Input_Mode==Input_BT1120_20BIT)
    {
	          lt8618_WriteI2C_Byte( 0xff, 0x81 );
			  lt8618_WriteI2C_Byte( 0x23, 0x40 );
			  lt8618_WriteI2C_Byte( 0x24, 0x61 );         //0x62(u3) ,0x64 icp set
			  lt8618_WriteI2C_Byte( 0x25, 0x00 );         //prediv=div(n+1)
			  lt8618_WriteI2C_Byte( 0x2c, 0x9e );
			  //if(INPUT_IDCK_CLK==_Less_than_50M)
			  if(dclk<50000)
				{
				  lt8618_WriteI2C_Byte( 0x2d, 0xaa );       //[5:4]divx_set //[1:0]freq set
				  smi_lt8618Msg("LT8618SX PLL LOW\n");
				} 
				//else if(INPUT_IDCK_CLK==_Bound_50_100M)
				else if((dclk>50000)&&(dclk<100000))
				{
				 lt8618_WriteI2C_Byte( 0x2d, 0x99 );       //[5:4] divx_set //[1:0]freq set
				 smi_lt8618Msg("LT8618SX PLL MID\n");
				}
				//else if(INPUT_IDCK_CLK==_Greater_than_100M)
				else
				{
				 lt8618_WriteI2C_Byte( 0x2d, 0x88 );       //[5:4] divx_set //[1:0]freq set
				 smi_lt8618Msg("LT8618SX PLL HIGH\n");
				}
			    lt8618_WriteI2C_Byte( 0x26, 0x55 );
			    lt8618_WriteI2C_Byte( 0x27, 0x66 );   //phase selection for d_clk
				lt8618_WriteI2C_Byte( 0x28, 0x88 );
			
			    lt8618_WriteI2C_Byte( 0x29, 0x04 );   //for U3 for U3 SDR/DDR fixed phase
			   
    }
		else if(Video_Input_Mode==Input_BT656_8BIT)
    {
              lt8618_WriteI2C_Byte( 0xff, 0x81 );
			  lt8618_WriteI2C_Byte( 0x23, 0x40 );
			  lt8618_WriteI2C_Byte( 0x24, 0x61 );         //icp set
			  lt8618_WriteI2C_Byte( 0x25, 0x00 );         //prediv=div(n+1)
			   lt8618_WriteI2C_Byte( 0x2c, 0x9e );
			  //if(INPUT_IDCK_CLK==_Less_than_50M)
			  if(dclk<50000)
				{
					lt8618_WriteI2C_Byte( 0x2d, 0xab );       //[5:4]divx_set //[1:0]freq set
				 	smi_lt8618Msg("LT8618SX PLL LOW\n");
				} 
				//else if(INPUT_IDCK_CLK==_Bound_50_100M)
				else if((dclk>50000)&&(dclk<100000))
				{
				 	lt8618_WriteI2C_Byte( 0x2d, 0x9a );       //[5:4] divx_set //[1:0]freq set
				 	smi_lt8618Msg("LT8618SX PLL MID\n");
				}
				//else if(INPUT_IDCK_CLK==_Greater_than_100M)
				else
				{
				 	lt8618_WriteI2C_Byte( 0x2d, 0x89 );       //[5:4] divx_set //[1:0]freq set
				 	smi_lt8618Msg("LT8618SX PLL HIGH\n");
				}
			  lt8618_WriteI2C_Byte( 0x26, 0x55 );
			  lt8618_WriteI2C_Byte( 0x27, 0x66 );   //phase selection for d_clk
			  lt8618_WriteI2C_Byte( 0x28, 0xa9 );
			
			  lt8618_WriteI2C_Byte( 0x29, 0x04 );   //for U3 for U3 SDR/DDR fixed phase
    }
		
		
				lt8618_WriteI2C_Byte( 0xff, 0x81 );
				read_val=lt8618_ReadI2C_Byte(0x2b);
				lt8618_WriteI2C_Byte( 0x2b, read_val&0xfd );// sw_en_txpll_cal_en
				read_val=lt8618_ReadI2C_Byte(0x2e);
				lt8618_WriteI2C_Byte( 0x2e, read_val&0xfe );//sw_en_txpll_iband_set
				
				lt8618_WriteI2C_Byte( 0xff, 0x82 );
				lt8618_WriteI2C_Byte( 0xde, 0x00 );
				lt8618_WriteI2C_Byte( 0xde, 0xc0 );
				
                lt8618_WriteI2C_Byte( 0xff, 0x80 );
				lt8618_WriteI2C_Byte( 0x16, 0xf1 );
				lt8618_WriteI2C_Byte( 0x18, 0xdc );//txpll _sw_rst_n
				lt8618_WriteI2C_Byte( 0x18, 0xfc );
				lt8618_WriteI2C_Byte( 0x16, 0xf3 );	
		 
				if(USE_DDRCLK == 1)
				{
					lt8618_WriteI2C_Byte( 0xff, 0x81 );
					lt8618_WriteI2C_Byte( 0x27, 0x60 );   //phase selection for d_clk
					lt8618_WriteI2C_Byte( 0x4d, 0x05 );//
					lt8618_WriteI2C_Byte( 0x2a, 0x10 );//
					lt8618_WriteI2C_Byte( 0x2a, 0x30 );//sync rest
				}
				else
				{
					lt8618_WriteI2C_Byte( 0xff, 0x81 );
					lt8618_WriteI2C_Byte( 0x27, 0x66 );   //phase selection for d_clk
					lt8618_WriteI2C_Byte( 0x2a, 0x00 );//
					lt8618_WriteI2C_Byte( 0x2a, 0x20 );//sync rest
				}
					  for(j=0;j<0x05;j++)
				    {
						mdelay(10);
						lt8618_WriteI2C_Byte(0xff,0x80);	
			            lt8618_WriteI2C_Byte(0x16,0xe3); /* pll lock logic reset */
			            lt8618_WriteI2C_Byte(0x16,0xf3);
							
						lt8618_WriteI2C_Byte( 0xff, 0x82 );
						lock=0x80&lt8618_ReadI2C_Byte(0x15);	
						cali_val=lt8618_ReadI2C_Byte(0xea);	
						 cali_done=0x80&lt8618_ReadI2C_Byte(0xeb);
							if(lock&&cali_done&&(cali_val!=0xff))
							{	
#ifdef _DEBUG_MODE
								lt8618_WriteI2C_Byte( 0xff, 0x82 );
								smi_lt8618Msg("0x8215=0x%x",lt8618_ReadI2C_Byte(0x15));
								smi_lt8618Msg("0x82e6=0x%x",lt8618_ReadI2C_Byte(0xe6));	
								smi_lt8618Msg("0x82e7=0x%x",lt8618_ReadI2C_Byte(0xe7));	
								smi_lt8618Msg("0x82e8=0x%x",lt8618_ReadI2C_Byte(0xe8));	
								smi_lt8618Msg("0x82e9=0x%x",lt8618_ReadI2C_Byte(0xe9));	
								smi_lt8618Msg("0x82ea=0x%x",lt8618_ReadI2C_Byte(0xea));	
								smi_lt8618Msg("0x82eb=0x%x",lt8618_ReadI2C_Byte(0xeb));	
								smi_lt8618Msg("0x82ec=0x%x",lt8618_ReadI2C_Byte(0xec));	
								smi_lt8618Msg("0x82ed=0x%x",lt8618_ReadI2C_Byte(0xed));	
								smi_lt8618Msg("0x82ee=0x%x",lt8618_ReadI2C_Byte(0xee));	
								smi_lt8618Msg("0x82ef=0x%x",lt8618_ReadI2C_Byte(0xef));
#endif										
								smi_lt8618Msg("TXPLL Lock\n");
								
								if(USE_DDRCLK == 1)
								{
								lt8618_WriteI2C_Byte( 0xff, 0x81 );
								lt8618_WriteI2C_Byte( 0x4d, 0x05 );//
								lt8618_WriteI2C_Byte( 0x2a, 0x10 );//
								lt8618_WriteI2C_Byte( 0x2a, 0x30 );//sync rest
								}
								else
								{
								lt8618_WriteI2C_Byte( 0xff, 0x81 );
								lt8618_WriteI2C_Byte( 0x2a, 0x00 );//
								lt8618_WriteI2C_Byte( 0x2a, 0x20 );//sync rest
								}
							return 1;
							}
							else
							{
							lt8618_WriteI2C_Byte( 0xff, 0x80 );
							lt8618_WriteI2C_Byte( 0x16, 0xf1 );
							lt8618_WriteI2C_Byte( 0x18, 0xdc );//txpll _sw_rst_n
							lt8618_WriteI2C_Byte( 0x18, 0xfc );
							lt8618_WriteI2C_Byte( 0x16, 0xf3 );	
							smi_lt8618Msg("TXPLL Reset\n");
							}
					  }
				smi_lt8618Msg("TXPLL Unlock\n");
				return 0;
}


int LT8618SX_PLL(void)
{
	u8 read_val=0;
	lt8618_WriteI2C_Byte(0xff,0x80);
	read_val=lt8618_ReadI2C_Byte(0x02);//get IC Version
    if(read_val==0xe1)
	{
	 LT8618SX_PLL_Version_U2();
	 smi_lt8618Msg("LT8618SX chip u2c\n");
	 return 0;
	}
	else if(read_val==0xe2)
	{
	 LT8618SX_PLL_Version_U3();
	 smi_lt8618Msg("LT8618SX chip u3c\n");
	 return 0;
	}
	else
	{
	 smi_lt8618Msg("LT8618SX fail version=%x\n",read_val);
	 return -1;
	}
}

void LT8618SX_Audio_Init(void)
{
    if(Audio_Input_Mode==I2S_2CH)
    {
       // IIS Input
	   		smi_lt8618Msg("LT8618SX Audio inut = I2S_2CH\n");
			lt8618_WriteI2C_Byte( 0xff, 0x82 );   // register bank
			lt8618_WriteI2C_Byte( 0xd6, Tx_Out_Mode|0x0e );   // bit7 = 0 : DVI output; bit7 = 1: HDMI output
			lt8618_WriteI2C_Byte( 0xd7, 0x04 );   
			
			lt8618_WriteI2C_Byte( 0xff, 0x84 );   // register bank
			lt8618_WriteI2C_Byte( 0x06, 0x08 );
			lt8618_WriteI2C_Byte( 0x07, 0x10 );	
			lt8618_WriteI2C_Byte( 0x10, 0x15 );
			lt8618_WriteI2C_Byte( 0x12, 0x60 );

			lt8618_WriteI2C_Byte( 0x0f, 0x0b + Sample_Freq[Sampling_rate] );
			if(Sampling_Size == _24_32bit)
				lt8618_WriteI2C_Byte( 0x34, 0xd4 );   //CTS_N / 2; 32bit(24bit)
			else
				lt8618_WriteI2C_Byte( 0x34, 0xd5 );	//CTS_N / 4; 16bit
			lt8618_WriteI2C_Byte( 0x35, (u8)( IIS_N[Sampling_rate] / 0x10000 ) );
			lt8618_WriteI2C_Byte( 0x36, (u8)( ( IIS_N[Sampling_rate] & 0x00FFFF ) / 0x100 ) );
			lt8618_WriteI2C_Byte( 0x37, (u8)( IIS_N[Sampling_rate] & 0x0000FF ) );
			lt8618_WriteI2C_Byte( 0x3c, 0x21 );   // Null packet enable
    }
    else if(Audio_Input_Mode==SPDIF)///
    {
			smi_lt8618Msg("LT8618SX Audio inut = SPDIF\n");
			lt8618_WriteI2C_Byte(0xff,0x84);
			lt8618_WriteI2C_Byte(0x06,0x0c);
			lt8618_WriteI2C_Byte(0x07,0x10);	
			lt8618_WriteI2C_Byte(0x34,0xd4); //CTS_N
    } 
}

void LT8618SX_IRQ_Init(void)
{
		lt8618_WriteI2C_Byte(0xff,0x82);
		lt8618_WriteI2C_Byte(0x10,0x00); //Output low level active;
		lt8618_WriteI2C_Byte(0x58,0x02); //Det HPD
		
		//lt8618_WriteI2C_Byte(0x9e,0xff); //vid chk clk
		lt8618_WriteI2C_Byte(0x9e,0xf7);
		
		lt8618_WriteI2C_Byte(0x00,0xfe);   //mask0 vid_change
#ifdef _HDCP
	  lt8618_WriteI2C_Byte(0x01,0xed);   //mask1 bit1:~tx_auth_pass bit4:tx_auth_done
#endif
	     lt8618_WriteI2C_Byte(0x03,0x3f); //mask3  //tx_det
	
		lt8618_WriteI2C_Byte(0x02,0xff); //mask2
	
	
		lt8618_WriteI2C_Byte(0x04,0xff); //clear0
		lt8618_WriteI2C_Byte(0x04,0xfe); //clear0
#ifdef _HDCP
	  lt8618_WriteI2C_Byte(0x05,0xff); //clear1
		lt8618_WriteI2C_Byte(0x05,0xed); //clear1
#endif		
		lt8618_WriteI2C_Byte(0x07,0xff); //clear3
		lt8618_WriteI2C_Byte(0x07,0x3f); //clear3
}

void LT8618SX_HDCP_Init( void )         //luodexing
{
	 u8 read_val=0;
	
	    lt8618_WriteI2C_Byte( 0xff, 0x80 );
	    lt8618_WriteI2C_Byte( 0x13, 0xf8 );
	    lt8618_WriteI2C_Byte( 0x13, 0xf9 );
	
	    lt8618_WriteI2C_Byte( 0xff, 0x85 );
		lt8618_WriteI2C_Byte( 0x15, 0x15 );//bit3
		lt8618_WriteI2C_Byte( 0x15, 0x05 );//0x45

	    lt8618_WriteI2C_Byte( 0xff, 0x85 );
	    lt8618_WriteI2C_Byte( 0x17, 0x0f );
	    lt8618_WriteI2C_Byte( 0x0c, 0x30 );
	   
	    lt8618_WriteI2C_Byte( 0xff, 0x82 );
	    read_val=lt8618_ReadI2C_Byte( 0xd6 );
	    smi_lt8618Msg("LT8618SX hdcp 0x82d6=0x%x",read_val);
	    if(read_val&0x80)//hdmi mode
			{
			lt8618_WriteI2C_Byte( 0xff, 0x85 );
			lt8618_WriteI2C_Byte( 0x13, 0x3c );//bit3
			}
			else//dvi mode
			{
			lt8618_WriteI2C_Byte( 0xff, 0x85 );
			lt8618_WriteI2C_Byte( 0x13, 0x34 );//bit3
			}
			
	  lt8618_WriteI2C_Byte(0xff,0x84);
      lt8618_WriteI2C_Byte(0x10,0x2c);
	  lt8618_WriteI2C_Byte(0x12,0x64);    

}

void LT8618SX_HDMI_Out_Enable(bool en)
{
	lt8618_WriteI2C_Byte(0xff,0x81);
	if(en)
		{
	    lt8618_WriteI2C_Byte(0x30,0xea);
	    smi_lt8618Msg("LT8618SX HDMI output Enable\n");
		}
	else
		{
	    lt8618_WriteI2C_Byte(0x30,0x00);
	    smi_lt8618Msg("LT8618SX HDMI output Disable\n");
		}
}

void LT8618SX_HDMI_TX_Phy(void) 
{
	lt8618_WriteI2C_Byte(0xff,0x81);
	lt8618_WriteI2C_Byte(0x30,0xea);//0xea
	lt8618_WriteI2C_Byte(0x31,0x44);//DC: 0x44, AC:0x73
	lt8618_WriteI2C_Byte(0x32,0x4a);
	lt8618_WriteI2C_Byte(0x33,0x0b);
	lt8618_WriteI2C_Byte(0x34,0x00);//d0 pre emphasis
	lt8618_WriteI2C_Byte(0x35,0x00);//d1 pre emphasis
	lt8618_WriteI2C_Byte(0x36,0x00);//d2 pre emphasis
	lt8618_WriteI2C_Byte(0x37,0x44);
	lt8618_WriteI2C_Byte(0x3f,0x0f);
	lt8618_WriteI2C_Byte(0x40,0xb0);//clk swing
	lt8618_WriteI2C_Byte(0x41,0xa0);//d0 swing
	lt8618_WriteI2C_Byte(0x42,0xa0);//d1 swing
	lt8618_WriteI2C_Byte(0x43,0xa0); //d2 swing
	lt8618_WriteI2C_Byte(0x44,0x0a);	
}

void LT8618sx_Pattern_Set(void)  
{
	  smi_lt8618Msg("LT8618sx Pattern Set\n");
	  LT8618SX_RST_PD_Init();
	  LT8618SX_TTL_Input_Analog();
	  LT8618SX_PLL();
	
	  //1080P60 pattern
		lt8618_WriteI2C_Byte(0xFF,0x82);
		lt8618_WriteI2C_Byte(0xa3,0x00); //de_delay
		lt8618_WriteI2C_Byte(0xa4,0xc0);
		lt8618_WriteI2C_Byte(0xa5,0x29); //de_top
		lt8618_WriteI2C_Byte(0xa6,0x07);
		lt8618_WriteI2C_Byte(0xa7,0x80); //de_cnt
		lt8618_WriteI2C_Byte(0xa8,0x04);
		lt8618_WriteI2C_Byte(0xa9,0x38); //de_line
		lt8618_WriteI2C_Byte(0xaa,0x08);
		lt8618_WriteI2C_Byte(0xab,0x98); //htotal
		lt8618_WriteI2C_Byte(0xac,0x04);
		lt8618_WriteI2C_Byte(0xad,0x65); //vototal
		lt8618_WriteI2C_Byte(0xae,0x00);
		lt8618_WriteI2C_Byte(0xaf,0x2c); //hvsa
	  lt8618_WriteI2C_Byte(0xb0,0x05);
	  
	  // pattern pixel clk
	  lt8618_WriteI2C_Byte(0xFF,0x83);
		lt8618_WriteI2C_Byte(0x2d,0x50); 
		lt8618_WriteI2C_Byte(0x26,0xb7);
		
		lt8618_WriteI2C_Byte(0xff,0x80); 
		lt8618_WriteI2C_Byte(0x11,0x5a);
		lt8618_WriteI2C_Byte(0x11,0xfa); 
		
		//select ad_txpll_d_clk
		lt8618_WriteI2C_Byte(0xff,0x82); 
		lt8618_WriteI2C_Byte(0x4f,0x80);
		lt8618_WriteI2C_Byte(0x50,0x20); 
		
		//avi
		lt8618_WriteI2C_Byte(0xff,0x84);
		lt8618_WriteI2C_Byte(0x43,0x26); 
		lt8618_WriteI2C_Byte(0x44,0x10);
		lt8618_WriteI2C_Byte(0x45,0x21); 
		lt8618_WriteI2C_Byte(0x47,0x10);
		
		LT8618SX_HDMI_TX_Phy();
		LT8618SX_HDMI_Out_Enable(1);
	
}

void LT8618SX_CSC(void)
{
	if(Video_Output_Mode == Output_RGB888)
	{
			HDMI_Y=0;
			lt8618_WriteI2C_Byte(0xff,0x82);
			if(Video_Input_Mode == Input_YCbCr444)
			{
				lt8618_WriteI2C_Byte(0xb9,0x0c); //0x08//YCbCr to RGB
			}
			else if(Video_Input_Mode==Input_YCbCr422_16BIT||
							Video_Input_Mode==Input_BT1120_16BIT||
							Video_Input_Mode==Input_BT1120_20BIT||
							Video_Input_Mode==Input_BT1120_24BIT||
							Video_Input_Mode==Input_BT656_8BIT ||
							Video_Input_Mode==Input_BT656_10BIT||
							Video_Input_Mode==Input_BT656_12BIT||
							Video_Input_Mode==Input_BT601_8BIT )
			{
				lt8618_WriteI2C_Byte(0xb9,0x1c); //0x18//YCbCr to RGB,YCbCr 422 convert to YCbCr 444
			}
			else
			{
			lt8618_WriteI2C_Byte(0xb9,0x00); //No csc
			}
	}
	else if(Video_Output_Mode == Output_YCbCr444)
	{
		 HDMI_Y = 2;
	}
	else if(Video_Output_Mode == Output_YCbCr422)
	{
		 HDMI_Y = 1;
	}
}

u8 LT8618SX_Input_Change(void)
{
	static u32 last_dclk_=0;
	u32 dclk_;
	
  lt8618_WriteI2C_Byte(0xff,0x82);
  lt8618_WriteI2C_Byte(0x17,0x80);
  mdelay(100);
  dclk_=(((lt8618_ReadI2C_Byte(0x1d))&0x0f)<<8)+lt8618_ReadI2C_Byte(0x1e);
  dclk_=(dclk_<<8)+lt8618_ReadI2C_Byte(0x1f);

	if(dclk_<10000) //<10MHz
	{
	return 0;
	}
	if(last_dclk_>=dclk_ )
	{
	  if((last_dclk_ - dclk_)<10000)
		{
			last_dclk_=dclk_;
		 return 0;
		}
		else
		{
			last_dclk_=dclk_;
		 return 1;
		}
	}
	else
	{
	  if((dclk_ - last_dclk_)<10000)
		{
			last_dclk_=dclk_;
		 return 0;
		}
		else
		{
			last_dclk_=dclk_;
		 return 1;
		}
	}
}

void LT8618SX_Print_Video_Inf(void)
{
	smi_lt8618Msg("##########################LT8618SX Input Infor#####################\n");
	smi_lt8618Msg("hfp = %u,hs_width = %u,hbp = %u,h_act = %u,h_tal = %u,vfp = %u,vs_width = %u,vbp = %u,v_act = %u, v_tal = %u\n",hfp,hs_width,hbp,h_act,h_tal,vfp,vs_width,vbp,v_act,v_tal);
	smi_lt8618Msg("---------------------------------------------------------\n");	
}


u8  LT8618SX_Video_Check(unsigned long width, unsigned long height)
{
   
  u8 temp;
	
	hfp = 0;
	hs_width = 0;
	hbp = 0;
	h_act = 0;
	h_tal = 0;
	vfp = 0;
	vs_width = 0;
	vbp = 0;
	v_act = 0;
	v_tal = 0;
	
	lt8618_WriteI2C_Byte( 0xff, 0x80 );
	lt8618_WriteI2C_Byte( 0x13, 0xf1 );//ttl video process reset
	lt8618_WriteI2C_Byte( 0x12, 0xfb );//video check reset
	mdelay( 1 ); 
	lt8618_WriteI2C_Byte( 0x12, 0xff );
	lt8618_WriteI2C_Byte( 0x13, 0xf9 );
	
	mdelay( 100 ); 
				
				
  if((Video_Input_Mode==Input_BT601_8BIT)||(Video_Input_Mode==Input_RGB888)||(Video_Input_Mode==Input_YCbCr422_16BIT)||(Video_Input_Mode==Input_YCbCr444))/*extern sync*/
  {
		lt8618_WriteI2C_Byte(0xff,0x82);
		lt8618_WriteI2C_Byte( 0x51, 0x00 );
		
       lt8618_WriteI2C_Byte(0xff,0x82); //video check
       temp=lt8618_ReadI2C_Byte(0x70);  //hs vs polarity
		
       if(temp&0x02)
			 {
			     smi_lt8618Msg("vs_pol is 1\n");
			 }
			 else
			 {
			     smi_lt8618Msg("vs_pol is 0\n");
			 }
       if( temp & 0x01 )
			 {
			     smi_lt8618Msg("hs_pol is 1\n");
			 }
			 else
			 {
			     smi_lt8618Msg("hs_pol is 0\n");
			 } 
			 
       vs_width = lt8618_ReadI2C_Byte(0x71);
       hs_width = lt8618_ReadI2C_Byte(0x72);
       hs_width = ( (hs_width & 0x0f) << 8 ) + lt8618_ReadI2C_Byte(0x73);
       vbp = lt8618_ReadI2C_Byte(0x74);
       vfp = lt8618_ReadI2C_Byte(0x75);
       hbp = lt8618_ReadI2C_Byte(0x76);
       hbp = ( (hbp & 0x0f) << 8 ) + lt8618_ReadI2C_Byte(0x77);
       hfp = lt8618_ReadI2C_Byte(0x78);
       hfp = ( (hfp & 0x0f) << 8 ) + lt8618_ReadI2C_Byte(0x79);
       v_tal = lt8618_ReadI2C_Byte(0x7a);
       v_tal = ( v_tal << 8 ) + lt8618_ReadI2C_Byte(0x7b);
       h_tal = lt8618_ReadI2C_Byte(0x7c);
       h_tal = ( h_tal << 8 ) + lt8618_ReadI2C_Byte(0x7d);
       v_act = lt8618_ReadI2C_Byte(0x7e);
       v_act = ( v_act << 8 ) + lt8618_ReadI2C_Byte(0x7f);
       h_act = lt8618_ReadI2C_Byte(0x80);
       h_act = ( h_act << 8 ) + lt8618_ReadI2C_Byte(0x81);
   }	  
  else if((Video_Input_Mode==Input_BT1120_20BIT)||(Video_Input_Mode==Input_BT1120_16BIT)||(Video_Input_Mode==Input_BT656_8BIT))/*embbedded sync */
	{
		lt8618_WriteI2C_Byte(0xff,0x82);
		lt8618_WriteI2C_Byte( 0x51, 0x42 );
		v_act = lt8618_ReadI2C_Byte(0x8b);
		v_act = ( v_act << 8 ) + lt8618_ReadI2C_Byte(0x8c);

		h_act = lt8618_ReadI2C_Byte(0x8d);
		h_act = ( h_act << 8 ) + lt8618_ReadI2C_Byte(0x8e)-0x04;/*note -0x04*/

		h_tal= lt8618_ReadI2C_Byte(0x8f);
		h_tal = ( h_tal << 8 ) + lt8618_ReadI2C_Byte(0x90);
	}
	   

    if(Video_Input_Mode==Input_BT601_8BIT||Video_Input_Mode==Input_BT656_8BIT)
    {
        hs_width/=2;
        hbp/=2;
        hfp/=2;
        h_tal/=2;
        h_act/=2;
    }
	
    LT8618SX_Print_Video_Inf();
	
	if(((h_act==video_720x480_60Hz.hact)&&
		 (v_act==video_720x480_60Hz.vact)&&
		 (h_tal==video_720x480_60Hz.htotal)) || (width==video_720x480_60Hz.hact && height==video_720x480_60Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_720x480_60Hz \n");
			Video_Format=video_720x480_60Hz_vic3;
			HDMI_VIC=3;
			video_bt=video_720x480_60Hz;
	  }	
    else if(((h_act==video_800x600_60Hz.hact)&&
		 (v_act==video_800x600_60Hz.vact)&&
		 (h_tal==video_800x600_60Hz.htotal)
	) || (width==video_800x600_60Hz.hact && height==video_800x600_60Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_800x600_60Hz \n");
			Video_Format=video_800x600_60Hz_vic;
			HDMI_VIC=0;
			video_bt=video_800x600_60Hz;
	  }
	else if(((h_act==video_1280x720_30Hz.hact)&&
		 (v_act==video_1280x720_30Hz.vact)&&
		 (h_tal==video_1280x720_30Hz.htotal)
	) || (width==video_1280x720_30Hz.hact && height==video_1280x720_30Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_1280x720_30Hz\n ");
			Video_Format=video_1280x720_30Hz_vic62;
			HDMI_VIC=62;
			video_bt=video_1280x720_30Hz;
	  }
	else if(((h_act==video_1280x720_60Hz.hact)&&
		 (v_act==video_1280x720_60Hz.vact)&&
		 (h_tal==video_1280x720_60Hz.htotal)
	 ) || (width==video_1280x720_60Hz.hact && height==video_1280x720_60Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_1280x720_60Hz \n");
			Video_Format=video_1280x720_60Hz_vic4;
			HDMI_VIC=4;
			video_bt=video_1280x720_60Hz;
	  }        
    else if(((h_act==video_1280x1024_60Hz.hact)&&
		 (v_act==video_1280x1024_60Hz.vact)&&
		 (h_tal==video_1280x1024_60Hz.htotal)
	) || (width==video_1280x1024_60Hz.hact && height==video_1280x1024_60Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_1280x1024_60Hz \n");
			Video_Format=video_1280x1024_60Hz_vic;
			HDMI_VIC=0;
			video_bt=video_1280x1024_60Hz;
	  }      
    else if(((h_act==video_1920x1080_60Hz.hact)&&
		 (v_act==video_1920x1080_60Hz.vact)&&
		 (h_tal==video_1920x1080_60Hz.htotal)) || (width==video_1920x1080_60Hz.hact && height==video_1920x1080_60Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_1920x1080_60Hz \n");
			Video_Format=video_1920x1080_60Hz_vic16;
			HDMI_VIC=16;
			video_bt=video_1920x1080_60Hz;
	  }   
	else if(((h_act==video_1680x1050_60Hz.hact)&&
		 (v_act==video_1680x1050_60Hz.vact)&&
		 (h_tal==video_1680x1050_60Hz.htotal)) || (width==video_1680x1050_60Hz.hact && height==video_1680x1050_60Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_1680x1050_60Hz \n");
			Video_Format=video_1680x1050_60Hz_vic;
			HDMI_VIC=0;
			video_bt=video_1680x1050_60Hz;
	  }    
    else if(((h_act==video_1440x900_60Hz.hact)&&
		 (v_act==video_1440x900_60Hz.vact)&&
		 (h_tal==video_1440x900_60Hz.htotal)) || (width==video_1440x900_60Hz.hact && height==video_1440x900_60Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_1440x900_60Hz \n");
			Video_Format=video_1400x900_60Hz_vic;
			HDMI_VIC=0;
			video_bt=video_1440x900_60Hz;
	  }      
    else if(((h_act==video_1366x768_60Hz.hact)&&
		 (v_act==video_1366x768_60Hz.vact)&&
		 (h_tal==video_1366x768_60Hz.htotal)) || (width==video_1366x768_60Hz.hact && height==video_1366x768_60Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_1366x768_60Hz\n ");
			Video_Format=video_1366x768_60Hz_vic;
			HDMI_VIC=0;
			video_bt=video_1366x768_60Hz;
	  }
    else if(((h_act==video_1280x960_60Hz.hact)&&
		 (v_act==video_1280x960_60Hz.vact)&&
		 (h_tal==video_1280x960_60Hz.htotal)) || (width==video_1280x960_60Hz.hact && height==video_1280x960_60Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_1280x960_60Hz\n ");
			Video_Format=video_1280x960_60Hz_vic;
			HDMI_VIC=0;
			video_bt=video_1280x960_60Hz;
	  }    
    else if(((h_act==video_1280x800_60Hz.hact)&&
		 (v_act==video_1280x800_60Hz.vact)&&
		 (h_tal==video_1280x800_60Hz.htotal)) || (width==video_1280x800_60Hz.hact && height==video_1280x800_60Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_1280x800_60Hz\n ");
			Video_Format=video_1280x800_60Hz_vic;
			HDMI_VIC=0;
			video_bt=video_1280x800_60Hz;
	  }
	else if(((h_act==video_3840x2160_30Hz.hact)&&
		 (v_act==video_3840x2160_30Hz.vact)&&
		 (h_tal==video_3840x2160_30Hz.htotal)) || (width==video_3840x2160_30Hz.hact && height==video_3840x2160_30Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_3840x2160_30Hz\n");
			Video_Format=video_3840x2160_30Hz_vic205;
			HDMI_VIC=16;
			video_bt=video_3840x2160_30Hz;
	  }
	else if(((h_act==video_1024x768_60Hz.hact)&&
		 (v_act==video_1024x768_60Hz.vact)&&
		 (h_tal==video_1024x768_60Hz.htotal)) || (width==video_1024x768_60Hz.hact && height==video_1024x768_60Hz.vact))
		{
			smi_lt8618Msg("Video_Check = video_1024x768_60Hz \n");
			Video_Format=video_1024x768_60Hz_vic;
			HDMI_VIC=0;
			video_bt=video_1024x768_60Hz;
	  }
	else
		{
			Video_Format=video_none;
		}
	return Video_Format;
}


void LT8618SX_HDMI_TX_Digital( void )
{
	//AVI
	u8	AVI_PB0	   = 0x00;
	u8	AVI_PB1	   = 0x00;
	u8	AVI_PB2	   = 0x00;

	AVI_PB1 = ( HDMI_Y << 5 );

	AVI_PB2 = 0x2A;  // 16:9
//	AVI_PB2 = 0x19;// 4:3

	AVI_PB0 = ( ( AVI_PB1 + AVI_PB2 + HDMI_VIC ) <= 0x6f ) ? ( 0x6f - AVI_PB1 - AVI_PB2 - HDMI_VIC ) : ( 0x16f - AVI_PB1 - AVI_PB2 - HDMI_VIC );

	lt8618_WriteI2C_Byte( 0xff, 0x84 );
	lt8618_WriteI2C_Byte( 0x43, AVI_PB0 );    //AVI_PB0
	lt8618_WriteI2C_Byte( 0x44, AVI_PB1 );    //AVI_PB1
	lt8618_WriteI2C_Byte( 0x45, AVI_PB2 );    //AVI_PB2
	lt8618_WriteI2C_Byte( 0x47, HDMI_VIC );   //AVI_PB4

	lt8618_WriteI2C_Byte( 0x10, 0x32 );       //data iland
	lt8618_WriteI2C_Byte( 0x12, 0x64 );       //act_h_blank

	//VS_IF, 4k 30hz need send VS_IF packet. Please refer to hdmi1.4 spec 8.2.3
	if( HDMI_VIC == 95 )
	{
//	   LT8618SXB_I2C_Write_Byte(0xff,0x84);
		lt8618_WriteI2C_Byte( 0x3d, 0x2a );   //UD1 infoframe enable

		lt8618_WriteI2C_Byte( 0x74, 0x81 );
		lt8618_WriteI2C_Byte( 0x75, 0x01 );
		lt8618_WriteI2C_Byte( 0x76, 0x05 );
		lt8618_WriteI2C_Byte( 0x77, 0x49 );
		lt8618_WriteI2C_Byte( 0x78, 0x03 );
		lt8618_WriteI2C_Byte( 0x79, 0x0c );
		lt8618_WriteI2C_Byte( 0x7a, 0x00 );
		lt8618_WriteI2C_Byte( 0x7b, 0x20 );
		lt8618_WriteI2C_Byte( 0x7c, 0x01 );
	}
	else
	{
//	   LT8618SXB_I2C_Write_Byte(0xff,0x84);
		lt8618_WriteI2C_Byte( 0x3d, 0x0a ); //UD1 infoframe disable
	}
    	//AVI_audio
    lt8618_WriteI2C_Byte( 0xff, 0x84 );
    lt8618_WriteI2C_Byte( 0xb2, 0x84 );
    lt8618_WriteI2C_Byte( 0xb3, 0x01 );
    lt8618_WriteI2C_Byte( 0xb4, 0x0a );
    lt8618_WriteI2C_Byte( 0xb5, 0x60 - ( ( ( Sampling_rate + 1 ) << 2 ) + Sampling_Size ) );  //checksum
    lt8618_WriteI2C_Byte( 0xb6, 0x11 );                                                       //AVI_PB0//LPCM
    lt8618_WriteI2C_Byte( 0xb7, ( ( Sampling_rate + 1 ) << 2 ) + Sampling_Size );             //AVI_PB1//32KHz 24bit(32bit)
}

void LT8618SX_BT_Set(void)
{
		if(Video_Input_Mode == Input_RGB888)
	{
		#ifdef Only_De_Mode
		
		lt8618_WriteI2C_Byte(0xff,0x82);
		lt8618_WriteI2C_Byte(0x2c,((video_bt.htotal)>>8)); 
		lt8618_WriteI2C_Byte(0x2d,(video_bt.htotal)); 
		lt8618_WriteI2C_Byte(0x2e,((video_bt.hact)>>8)); 
		lt8618_WriteI2C_Byte(0x2f,(video_bt.hact)); 
		lt8618_WriteI2C_Byte(0x30,((video_bt.hfp)>>8)); 
		lt8618_WriteI2C_Byte(0x31,(video_bt.hfp)); 
    	lt8618_WriteI2C_Byte(0x32,((video_bt.hbp)>>8)); 
		lt8618_WriteI2C_Byte(0x33,(video_bt.hbp)); 
		lt8618_WriteI2C_Byte(0x34,((video_bt.hs)>>8)); 
		lt8618_WriteI2C_Byte(0x35,(video_bt.hs));
		
		lt8618_WriteI2C_Byte(0x36,(video_bt.vact>>8));
		lt8618_WriteI2C_Byte(0x37,video_bt.vact);
		lt8618_WriteI2C_Byte(0x38,(video_bt.vfp>>8));
		lt8618_WriteI2C_Byte(0x39,video_bt.vfp);
		lt8618_WriteI2C_Byte(0x3a,(video_bt.vbp>>8));
		lt8618_WriteI2C_Byte(0x3b,video_bt.vbp);
		lt8618_WriteI2C_Byte(0x3c,(video_bt.vs>>8));
		lt8618_WriteI2C_Byte(0x3d,video_bt.vs);
		#endif
	}
	else if((Video_Input_Mode == Input_BT1120_16BIT) || (Video_Input_Mode == Input_BT1120_20BIT))
	{
		lt8618_WriteI2C_Byte(0xff,0x82);
		lt8618_WriteI2C_Byte(0x20,((video_bt.hact)>>8)); 
		lt8618_WriteI2C_Byte(0x21,(video_bt.hact)); 
		lt8618_WriteI2C_Byte(0x22,((video_bt.hfp)>>8)); 
		lt8618_WriteI2C_Byte(0x23,(video_bt.hfp)); 
		lt8618_WriteI2C_Byte(0x24,((video_bt.hs)>>8)); 
		lt8618_WriteI2C_Byte(0x25,(video_bt.hs)); 
	
		lt8618_WriteI2C_Byte(0x36,(video_bt.vact>>8));
		lt8618_WriteI2C_Byte(0x37,video_bt.vact);
		lt8618_WriteI2C_Byte(0x38,(video_bt.vfp>>8));
		lt8618_WriteI2C_Byte(0x39,video_bt.vfp);
		lt8618_WriteI2C_Byte(0x3a,(video_bt.vbp>>8));
		lt8618_WriteI2C_Byte(0x3b,video_bt.vbp);
		lt8618_WriteI2C_Byte(0x3c,(video_bt.vs>>8));
		lt8618_WriteI2C_Byte(0x3d,video_bt.vs);
	}
	else if(Video_Input_Mode == Input_BT601_8BIT)
	{
			lt8618_WriteI2C_Byte( 0xff, 0x82 );
			lt8618_WriteI2C_Byte( 0x2c,((2*video_bt.htotal)>>8));
			lt8618_WriteI2C_Byte( 0x2d,(2*video_bt.htotal));
			lt8618_WriteI2C_Byte( 0x2e,((2*video_bt.hact)>>8));
			lt8618_WriteI2C_Byte( 0x2f,(2*video_bt.hact));
			lt8618_WriteI2C_Byte( 0x30,((2*video_bt.hfp)>>8));
			lt8618_WriteI2C_Byte( 0x31,(2*video_bt.hfp));
			lt8618_WriteI2C_Byte( 0x32,((2*video_bt.hbp)>>8));
			lt8618_WriteI2C_Byte( 0x33,(2*video_bt.hbp));
			lt8618_WriteI2C_Byte( 0x34,((2*video_bt.hs)>>8));
			lt8618_WriteI2C_Byte( 0x35,(2*video_bt.hs));

			lt8618_WriteI2C_Byte( 0x36,((video_bt.vact)>>8));
			lt8618_WriteI2C_Byte( 0x37,(video_bt.vact));
			lt8618_WriteI2C_Byte( 0x38,((video_bt.vfp)>>8));
			lt8618_WriteI2C_Byte( 0x39,(video_bt.vfp));
			lt8618_WriteI2C_Byte( 0x3a,((video_bt.vbp)>>8));
			lt8618_WriteI2C_Byte( 0x3b,(video_bt.vbp));
			lt8618_WriteI2C_Byte( 0x3c,((video_bt.vs)>>8));
			lt8618_WriteI2C_Byte( 0x3d,(video_bt.vs));
	}
	else if(Video_Input_Mode == Input_BT656_8BIT)
	{
	    video_bt.hs *= 2;
        video_bt.hbp *= 2;
        video_bt.hfp *= 2;
        video_bt.htotal *= 2;
        video_bt.hact *= 2;
			  
    	lt8618_WriteI2C_Byte(0xff,0x82);
		lt8618_WriteI2C_Byte(0x20,((video_bt.hact)>>8)); 
		lt8618_WriteI2C_Byte(0x21,(video_bt.hact)); 
		lt8618_WriteI2C_Byte(0x22,((video_bt.hfp)>>8)); 
		lt8618_WriteI2C_Byte(0x23,(video_bt.hfp)); 
		lt8618_WriteI2C_Byte(0x24,((video_bt.hs)>>8)); 
		lt8618_WriteI2C_Byte(0x25,(video_bt.hs)); 
	
		lt8618_WriteI2C_Byte(0x36,(video_bt.vact>>8));
		lt8618_WriteI2C_Byte(0x37,video_bt.vact);
		lt8618_WriteI2C_Byte(0x38,(video_bt.vfp>>8));
		lt8618_WriteI2C_Byte(0x39,video_bt.vfp);
		lt8618_WriteI2C_Byte(0x3a,(video_bt.vbp>>8));
		lt8618_WriteI2C_Byte(0x3b,video_bt.vbp);
		lt8618_WriteI2C_Byte(0x3c,(video_bt.vs>>8));
		lt8618_WriteI2C_Byte(0x3d,video_bt.vs);
	
	}
}

u8 LT8618SX_Phase(void)  
{
  u8 temp=0;
	u8 read_value=0;
	u8 b_ok=0;
	u8 Temp_f=0;

	for(temp=0;temp<0x0a;temp++)
	{
	  lt8618_WriteI2C_Byte(0xff,0x81); 
	  lt8618_WriteI2C_Byte(0x27,(0x60+temp));
		if(USE_DDRCLK ==0 )
		{
		lt8618_WriteI2C_Byte(0x4d,0x01); //sdr=01,ddr=05
		lt8618_WriteI2C_Byte(0x4d,0x09); //sdr=09,ddr=0d;
		}
		else
		{
		lt8618_WriteI2C_Byte(0x4d,0x05); //sdr=01,ddr=05
		lt8618_WriteI2C_Byte(0x4d,0x0d); //sdr=09,ddr=0d;
		}
		read_value=lt8618_ReadI2C_Byte(0x50);//1->0 
#ifdef _DEBUG_MODE
		smi_lt8618Msg("temp=%u\n",temp);
		smi_lt8618Msg("read_value=%u\n",read_value);
#endif
		if(read_value==0x00)
		{
		   if(b_ok==0)
		   {
		    Temp_f=temp;
		   }
		   b_ok=1;
		}
		else
		{
		   b_ok=0;
		}
	}
#ifdef _DEBUG_MODE
	smi_lt8618Msg("Temp_f=%u\n",Temp_f);
#endif
	return Temp_f;
}

/**********************************************************/
//only use for embbedded_sync(bt1120,bt656...) or DDR
/************************************************************/
 bool LT8618SX_Phase_Config(void)
{
	u8		Temp       = 0x00;
	u8    Temp_f       =0x00;
	u8	    OK_CNT	   = 0x00;
	u8      OK_CNT_1   = 0x00;
	u8      OK_CNT_2   = 0x00;
	u8      OK_CNT_3   = 0x00;	
	u8		Jump_CNT   = 0x00;
	u8		Jump_Num   = 0x00;
	u8		Jump_Num_1 = 0x00;
	u8		Jump_Num_2 = 0x00;
	u8		Jump_Num_3 = 0x00;
	bool    temp0_ok   =0;
	bool    temp9_ok   =0;
	bool	b_OK	   = 0;
	u16		V_ACT	   = 0x0000;
	u16		H_ACT	   = 0x0000;
	u16		H_TOTAL	   = 0x0000;
	u16		V_TOTAL	   = 0x0000;
	
	
	  Temp_f=LT8618SX_Phase() ;//it's setted before video check
		
		if(Video_Input_Mode == Input_RGB888)
		{
		lt8618_WriteI2C_Byte( 0xff, 0x81 );
		lt8618_WriteI2C_Byte( 0x27, ( 0x60 + (Temp_f+phease_offset)%0x0a ));
		smi_lt8618Msg("cail phase is 0x%x",(u16)lt8618_ReadI2C_Byte(0x27));
		return 1;
		}
		 
		while( Temp <= 0x09 )
		{	
				lt8618_WriteI2C_Byte( 0xff, 0x81 );
				lt8618_WriteI2C_Byte( 0x27, (0x60+Temp));
			
				lt8618_WriteI2C_Byte( 0xff, 0x80 );
				lt8618_WriteI2C_Byte( 0x13, 0xf1 );//ttl video process reset
				lt8618_WriteI2C_Byte( 0x12, 0xfb );//video check reset
				mdelay( 1 ); 
				lt8618_WriteI2C_Byte( 0x12, 0xff );
				lt8618_WriteI2C_Byte( 0x13, 0xf9 );
				mdelay( 100 ); //       
				if((Video_Input_Mode == Input_BT1120_20BIT)||(Video_Input_Mode == Input_BT1120_16BIT)||(Video_Input_Mode == Input_BT656_8BIT))
				{
						lt8618_WriteI2C_Byte( 0xff, 0x82 );
						lt8618_WriteI2C_Byte( 0x51, 0x42 );
						H_TOTAL	   = lt8618_ReadI2C_Byte( 0x8f );
						H_TOTAL	   = ( H_TOTAL << 8 ) + lt8618_ReadI2C_Byte( 0x90 );
						V_ACT  = lt8618_ReadI2C_Byte( 0x8b );
						V_ACT  = ( V_ACT << 8 ) + lt8618_ReadI2C_Byte( 0x8c );
						H_ACT  = lt8618_ReadI2C_Byte( 0x8d );
						H_ACT  = ( H_ACT << 8 ) + lt8618_ReadI2C_Byte( 0x8e )-0x04;//note
				}
#ifdef _DEBUG_MODE
						smi_lt8618Msg("\r\n h_total= %u\n",H_TOTAL);
						smi_lt8618Msg("\r\n v_act= %u\n",V_ACT);
						smi_lt8618Msg("\r\n h_act=%u\n",H_ACT);
#endif
			if( ( V_ACT ==(video_bt.vact) ) &&(H_ACT==(video_bt.hact) )&&(H_TOTAL== (video_bt.htotal) ))
				{
					OK_CNT++;
					if( b_OK == 0 )
					{
						b_OK = 1;
						Jump_CNT++;
						if( Jump_CNT == 1 )
						{
							Jump_Num_1 = Temp;
						}
						else if( Jump_CNT == 3 )
						{
							Jump_Num_2 = Temp;
						}
						else if( Jump_CNT == 5 )
						{
							Jump_Num_3 = Temp;
						}
					}

					if(Jump_CNT==1)
					{
					  OK_CNT_1++;
					}
					else if(Jump_CNT==3)
					{
					  OK_CNT_2++;
					}
					else if(Jump_CNT==5)
					{
					  OK_CNT_3++;
					}

					if(Temp==0)
					{
					  temp0_ok=1;
					}
					if(Temp==9)
					{
					  Jump_CNT++;
					  temp9_ok=1;
					}
#ifdef _DEBUG_MODE
					smi_lt8618Msg("this phase is ok,temp=%u\n",Temp);
					smi_lt8618Msg("\r\n Jump_CNT=%u",Jump_CNT);
#endif
				}
			else			
				{
					if( b_OK )
					{
						b_OK = 0;
						Jump_CNT++;
					}
					#ifdef _DEBUG_MODE
					smi_lt8618Msg("this phase is fail,temp=%u\n",Temp);
					smi_lt8618Msg("Jump_CNT=%u\n",Jump_CNT);
					#endif
				}
			Temp++;
			}
#ifdef _DEBUG_MODE
	    smi_lt8618Msg("OK_CNT_1=%u\n",OK_CNT_1);
		smi_lt8618Msg("OK_CNT_2=%u\n",OK_CNT_2);
		smi_lt8618Msg("OK_CNT_3=%u\n",OK_CNT_3);
		
#endif
		if((Jump_CNT==0)||(Jump_CNT>6))
		{
		smi_lt8618Msg("cali phase fail\n");
		return 0;
		}

		if((temp9_ok==1)&&(temp0_ok==1))
		{
		  if(Jump_CNT==6)
		  {
		  OK_CNT_3=OK_CNT_3+OK_CNT_1;
		  OK_CNT_1=0;
		  }
		  else if(Jump_CNT==4)
		  {
		   OK_CNT_2=OK_CNT_2+OK_CNT_1;
		   OK_CNT_1=0;
		  }
		}
		if(Jump_CNT>=2)
		{
			if(OK_CNT_1>=OK_CNT_2)
			{
			
				if(OK_CNT_1>=OK_CNT_3)
				{
					OK_CNT=OK_CNT_1;
					Jump_Num=Jump_Num_1;
				}
				else
				{
					OK_CNT=OK_CNT_3;
					Jump_Num=Jump_Num_3;
				}
			}
		
			else
			{
				if(OK_CNT_2>=OK_CNT_3)
				{
					OK_CNT=OK_CNT_2;
					Jump_Num=Jump_Num_2;
				}
				else
				{
					OK_CNT=OK_CNT_3;
					Jump_Num=Jump_Num_3;
				}
			}
		
	    }	  
	  lt8618_WriteI2C_Byte( 0xff, 0x81 );
			
		if( ( Jump_CNT == 2 ) || ( Jump_CNT == 4 ) || ( Jump_CNT == 6 ))
		{
			lt8618_WriteI2C_Byte( 0x27, ( 0x60 + ( Jump_Num  + ( OK_CNT / 2 ) ) % 0x0a ) );
		}
		if(OK_CNT==0x0a)
		{
			 lt8618_WriteI2C_Byte( 0x27, ( 0x60 + (Temp_f+phease_offset)%0x0a ));
		
		}
		smi_lt8618Msg("cail phase is 0x%x\n",(u16)lt8618_ReadI2C_Byte(0x27));
		return 1;
		
}

int LT8618SX_Task(unsigned long width, unsigned long height)
{
//static u8 timeout=0;
     int ret = 0; 
     smi_lt8618Msg("->LT8618SX_Task Start\n");
	  if( LT8618SX_Input_Change() || ( ( vid_chk_flag )&&( !LT8618SX_Input_Change() ))) 
		//if(vid_chk_flag)
		{     
			// timeout++;
			ret = LT8618SX_PLL();
			//LT8618SX_test_clk();//only for debug
			LT8618SX_Video_Check(width, height);
			smi_lt8618Msg("Video_Format = 0x%x\n",Video_Format);
			if((Video_Format == video_none))
          {
			vid_chk_flag=1;
            LT8618SX_HDMI_Out_Enable(0);
            #ifdef _HDCP
					  LT8618SX_HDCP_Enable(Disable);
            #endif	
			ret = -1; 
          }					
			else
			{
				#ifdef _READ_EDID
				LT8618SX_Output_Mode();//set ouput as DVI/HDMI
				#endif
				LT8618SX_CSC();
				LT8618SX_HDMI_TX_Digital();
				LT8618SX_HDMI_TX_Phy();
				LT8618SX_BT_Set();	
				LT8618SX_HDMI_Out_Enable(1);
			#ifdef _HDCP
				mdelay(200);
				LT8618SX_HDCP_Enable(Enable);
			#endif	
				LT8618SX_Phase_Config();
				
				
				vid_chk_flag=0;
				intb_flag=0;
				//timeout=0;
			}
	}
	smi_lt8618Msg("<-LT8618SX_Task Over\n");
	return ret;
}

void LT8618SX_Init(void)
{	
	LT8618SX_RST_PD_Init();
	LT8618SX_TTL_Input_Analog();
	LT8618SX_TTL_Input_Digital();
	LT8618SX_PLL();
	LT8618SX_Audio_Init();
	mdelay(1000);
#ifdef _HDCP
	LT8618SX_HDCP_Init();
	smi_lt8618Msg("LT8618SX HDCP Init\n");
#endif
	LT8618SX_IRQ_Init();//interrupt of hdp_change or video_input_change
	vid_chk_flag = 1;
}



void LT8618SX_Chip_ID(void)
{
    lt8618_WriteI2C_Byte(0xFF,0x80);
    lt8618_WriteI2C_Byte(0xee,0x01);
    smi_lt8618Msg("LT8618SX Chip ID = 0x%x 0x%x 0x%x\n", lt8618_ReadI2C_Byte(0x00), lt8618_ReadI2C_Byte(0x01), lt8618_ReadI2C_Byte(0x02));
}

void smi_iicGpioInit(unsigned char scl, unsigned char sda)
{
#ifdef I2CHW
	ddk768_hwI2CInit(0);
#else
	ddk768_swI2CInit(scl, sda);
#endif
}

void smi_lt8618Init(void)
{
	smi_iicGpioInit(30, 31);
	LT8618SX_Chip_ID();
#ifdef _PATTERN_TEST
	LT8618sx_Pattern_Set();
#else
	LT8618SX_Init();
	//LT8618SX_Task();
#endif
#ifdef I2CHW
    ddk768_hwI2CClose(0);
#endif
	smi_lt8618Msg("LT8618SX initialized\n");
}

int smi_lt8618SX_Task(unsigned long width, unsigned long height)
{
	smi_iicGpioInit(30, 31);
	LT8618SX_Task(width, height);
#ifdef I2CHW
    ddk768_hwI2CClose(0);
#endif
	return 0;
}
void LT8618SX_Output_Mode(char AudiOn)
{
	lt8618_WriteI2C_Byte(0xff, 0x82);
	if (AudiOn)//set hdmi mode
		lt8618_WriteI2C_Byte(0xd6, 0x8e);
	else//set dvi mode
		lt8618_WriteI2C_Byte(0xd6, 0x0e);

}