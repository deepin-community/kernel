
#include <linux/string.h>
#include "ddk768_reg.h"
#include "ddk768_help.h"
#include "ddk768_swi2c.h"
#include "ddk768_hwi2c.h"
#include "wm8978.h"

static unsigned short WM8978_REGVAL[58]=
{
	0X0000,0X0000,0X0000,0X0000,0X0050,0X0000,0X0140,0X0000,
	0X0000,0X0000,0X0000,0X00FF,0X00FF,0X0000,0X0100,0X00FF,
	0X00FF,0X0000,0X012C,0X002C,0X002C,0X002C,0X002C,0X0000,
	0X0032,0X0000,0X0000,0X0000,0X0000,0X0000,0X0000,0X0000,
	0X0038,0X000B,0X0032,0X0000,0X0008,0X000C,0X0093,0X00E9,
	0X0000,0X0000,0X0000,0X0000,0X0003,0X0010,0X0010,0X0100,
	0X0100,0X0002,0X0001,0X0001,0X0039,0X0039,0X0039,0X0039,
	0X0001,0X0001
}; 

unsigned char WM8978_Write_Reg(unsigned char reg, unsigned short val)
{
	unsigned char res;
	unsigned char RegAddr;
	unsigned char RegValue;
	RegAddr = (reg<<1)|((unsigned char)((val>>8)&0x01));
	RegValue = (unsigned char)val;
	if(!hwi2c_en)
		res = ddk768_swI2CWriteReg(WM8978_ADDR, RegAddr, RegValue);
	else
		res = ddk768_hwI2CWriteReg(0, WM8978_ADDR, RegAddr, RegValue);
	
	if(res == 0)
		WM8978_REGVAL[reg]=val;
	return res;
}

unsigned short WM8978_Read_Reg(unsigned char reg)
{  
	return WM8978_REGVAL[reg];	
} 

unsigned char WM8978_Init(void)
{
	unsigned char Res;

	if(hwi2c_en)
		ddk768_hwI2CInit(0);
	else
		ddk768_swI2CInit(30, 31);

	
	Res = WM8978_Write_Reg(0, 0);	//软复位WM8978
	if(Res)
		return 1;				     //发送指令失败,WM8978异常
	/* Set volume to 0 can improve the noise when init codec */
	WM8978_HPvol_Set(0, 0);   //耳机音量0-63（左和右分开设置）
	WM8978_SPKvol_Set(0);      //喇叭音量0-63
	WM8978_Write_Reg(1, 0x1B);  //R1,MICEN设置为1(MIC使能),BIASEN设置为1(模拟器工作),VMIDSEL[1:0]设置为:11(5K)
	WM8978_Write_Reg(2, 0x1B0); //R2,ROUT1,LOUT1输出使能(耳机可以工作),BOOSTENR,BOOSTENL使能
	WM8978_Write_Reg(3, 0x6C);  //R3,LOUT2,ROUT2输出使能(喇叭工作),RMIX,LMIX使能	
	WM8978_Write_Reg(6, 0);	    //R6,MCLK由外部提供
	WM8978_Write_Reg(43, 1<<4);//R43,INVROUT2反向,驱动喇叭
	WM8978_Write_Reg(47, 1<<8);//R47设置,PGABOOSTL,左通道MIC获得20倍增益
	WM8978_Write_Reg(48, 1<<8);//R48设置,PGABOOSTR,右通道MIC获得20倍增益
	WM8978_Write_Reg(49, 1<<1);//R49,TSDEN,开启过热保护 
	WM8978_Write_Reg(10, 1<<3);//R10,SOFTMUTE关闭,128x采样,最佳SNR 
	WM8978_Write_Reg(14, 1<<3);//R14,ADC 128x采样率
	
	/* Playback and record setup */

	WM8978_I2S_Cfg(2, 0);	    //设置I2S接口模式，数据位数不需要设置，播放从设备不使用
	WM8978_ADDA_Cfg(1, 1);	    //开启DAC和ADC
	WM8978_Input_Cfg(1, 1, 1);  //开启Line in输入通道，MIC和AUX
	WM8978_MIC_Gain(20);		//MIC增益设置,MIC录制时可开启
	WM8978_Output_Cfg(1, 0);	//开启DAC输出，关闭BYPASS输出  

	/* Make sure the IIC is idle when do this operation */
	WM8978_HPvol_Set(50, 50);   
	WM8978_SPKvol_Set(50);  
    
	return 0;
}

void WM8978_DeInit(void)
{
	if(hwi2c_en)
		ddk768_hwI2CClose(0);
	else
		ddk768_swI2CInit(30, 31);

	/* To Do: Here should be read device register not globle array.*/
	WM8978_Write_Reg(0, 0);
}

//WM8978 DAC/ADC配置
//adcen:adc使能(1)/关闭(0)
//dacen:dac使能(1)/关闭(0)
void WM8978_ADDA_Cfg(unsigned char dacen, unsigned char adcen)
{
	unsigned short regval;
	regval = WM8978_Read_Reg(3);	//读取R3
	if(dacen)
		regval |= 3<<0;			//R3最低2个位设置为1,开启DACR&DACL
	else 
		regval &= ~(3<<0);			//R3最低2个位清零,关闭DACR&DACL.
	WM8978_Write_Reg(3, regval);	//设置R3
	regval = WM8978_Read_Reg(2);	//读取R2
	if(adcen)
		regval |= 3<<0;			//R2最低2个位设置为1,开启ADCR&ADCL
	else 
		regval &= ~(3<<0);			//R2最低2个位清零,关闭ADCR&ADCL.
	WM8978_Write_Reg(2, regval);	//设置R2	
}

//WM8978 输入通道配置 
//micen:MIC开启(1)/关闭(0)
//lineinen:Line In开启(1)/关闭(0)
//auxen:aux开启(1)/关闭(0) 
void WM8978_Input_Cfg(unsigned char micen, unsigned char lineinen, unsigned char auxen)
{
	unsigned short regval;  
	regval = WM8978_Read_Reg(2);  //读取R2
	if(micen)
		regval |= 3<<2;			 //开启INPPGAENR,INPPGAENL(MIC的PGA放大)
	else 
		regval &= ~(3<<2);			 //关闭INPPGAENR,INPPGAENL.
 	WM8978_Write_Reg(2, regval);	 //设置R2 
	regval = WM8978_Read_Reg(44);//读取R44
	if(micen)
		regval |= 3<<4|3<<0;		//开启LIN2INPPGA,LIP2INPGA,RIN2INPPGA,RIP2INPGA.
	else 
		regval &= ~(3<<4|3<<0);	//关闭LIN2INPPGA,LIP2INPGA,RIN2INPPGA,RIP2INPGA.
	WM8978_Write_Reg(44, regval);	//设置R44
	if(lineinen)
		WM8978_LINEIN_Gain(5);	//LINE IN 0dB增益
	else 
		WM8978_LINEIN_Gain(0);	//关闭LINE IN
	if(auxen)
		WM8978_AUX_Gain(7);//AUX 6dB增益
	else 
		WM8978_AUX_Gain(0);//关闭AUX输入  
}

//WM8978 MIC增益设置(不包括BOOST的20dB,MIC-->ADC输入部分的增益)
//gain:0~63,对应-12dB~35.25dB,0.75dB/Step
void WM8978_MIC_Gain(unsigned char gain)
{
	gain &= 0x3F;
	WM8978_Write_Reg(45, gain);		//R45,左通道PGA设置 
	WM8978_Write_Reg(46, gain|1<<8);	//R46,右通道PGA设置
}

//WM8978 L2/R2(也就是Line In)增益设置(L2/R2-->ADC输入部分的增益)
//gain:0~7,0表示通道禁止,1~7,对应-12dB~6dB,3dB/Step
void WM8978_LINEIN_Gain(unsigned char gain)
{
	unsigned short regval;
	gain &= 0x07;
	regval = WM8978_Read_Reg(47);		//读取R47
	regval &= ~(7<<4);						//清除原来的设置 
 	WM8978_Write_Reg(47, regval|gain<<4);	//设置R47
	regval = WM8978_Read_Reg(48);		//读取R48
	regval &= ~(7<<4);						//清除原来的设置 
 	WM8978_Write_Reg(48,regval|gain<<4);	//设置R48
} 

//WM8978 AUXR,AUXL(PWM音频部分)增益设置(AUXR/L-->ADC输入部分的增益)
//gain:0~7,0表示通道禁止,1~7,对应-12dB~6dB,3dB/Step
void WM8978_AUX_Gain(unsigned char gain)
{
	unsigned short regval;
	gain &= 0x07;
	regval = WM8978_Read_Reg(47);		//读取R47
	regval &= ~(7<<0);						//清除原来的设置 
 	WM8978_Write_Reg(47, regval|gain<<0);	//设置R47
	regval = WM8978_Read_Reg(48);		//读取R48
	regval &= ~(7<<0);						//清除原来的设置 
 	WM8978_Write_Reg(48, regval|gain<<0);	//设置R48
}  

//WM8978 输出配置 
//dacen:DAC输出(放音)开启(1)/关闭(0)
//bpsen:Bypass输出(录音,包括MIC,LINE IN,AUX等)开启(1)/关闭(0)
void WM8978_Output_Cfg(unsigned char dacen, unsigned char bpsen)
{
	unsigned short regval = 0;
	if(dacen)
		regval |= 1<<0; //DAC输出使能
	if(bpsen)
	{
		regval |= 1<<1; //BYPASS使能
		regval |= 5<<2; //0dB增益
	} 
	WM8978_Write_Reg(50,regval); //R50设置
	WM8978_Write_Reg(51,regval); //R51设置 
}

//设置耳机左右声道音量
//voll:左声道音量(0~63)
//volr:右声道音量(0~63)
void WM8978_HPvol_Set(unsigned char voll, unsigned char volr)
{
	voll &= 0x3F;
	volr &= 0x3F;				//限定范围
	if(voll == 0)voll |= 1<<6;	//音量为0时,直接mute
	if(volr == 0)volr |= 1<<6;	//音量为0时,直接mute 
	WM8978_Write_Reg(52, voll);	//R52,耳机左声道音量设置
	WM8978_Write_Reg(53, volr|(1<<8)); //R53,耳机右声道音量设置,同步更新(HPVU=1)
}

//设置喇叭音量
//voll:左声道音量(0~63)
void WM8978_SPKvol_Set(unsigned char volx)
{
	volx &= 0x3F;//限定范围
	if(volx == 0)volx |= 1<<6;		//音量为0时,直接mute 
 	WM8978_Write_Reg(54, volx);	//R54,喇叭左声道音量设置
	WM8978_Write_Reg(55, volx|(1<<8)); //R55,喇叭右声道音量设置,同步更新(SPKVU=1)
}

//设置I2S工作模式
//fmt:0,LSB(右对齐);1,MSB(左对齐);2,飞利浦标准I2S;3,PCM/DSP;
//len:0,16位;1,20位;2,24位;3,32位;
void WM8978_I2S_Cfg(unsigned char fmt, unsigned char len)
{
	fmt &= 0x03;
	len &= 0x03; //限定范围
	WM8978_Write_Reg(4, (fmt<<3)|(len<<5)); //R4,WM8978工作模式设置	
	
}
