#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/io.h>


#include "ddk768_power.h"
#include "ddk768_intr.h"
#include "ddkdebug.h"
#include "ddk768_edid.h"
#include "ddk768_timer.h"


#include "ddk768_ep952.h"

#include "ddk768_swi2c.h"
#include "../ddk750/ddk750_swi2c.h"


#include "ddk768_help.h"
#include "ddk768_display.h"
#include "ddk768_reg.h"

#include "ep952api.h"
#include "ep952controller.h"  // HDMI Transmiter
#include "ep952settingsdata.h"



#ifdef HDMI_DEBUG
#define HDMI_DEBUG_Printf   printk
#else
#define HDMI_DEBUG_Printf(fmt,...)
#endif

int is_SM768 = 0;

void EP952_Register_Message(void);



EP952C_REGISTER_MAP EP952C_Registers = {0};
// Private Data
unsigned char IIC_EP952_Addr, IIC_HDCPKey_Addr;
unsigned short TempUSHORT;
//unsigned char Temp_Data[15];

// Global date for HDMI Transmiter
unsigned char is_RSEN;
unsigned char Cache_EP952_DE_Control;

// Private Functions
static unsigned char IIC_Write(unsigned char IICAddr, unsigned char ByteAddr, unsigned char *Data, unsigned int Size);
static unsigned char IIC_Read(unsigned char IICAddr, unsigned char ByteAddr, unsigned char *Data, unsigned int Size);

//==================================================================================================
//
// Private Functions
//

static unsigned char IIC_Write(unsigned char IICAddr, unsigned char ByteAddr, unsigned char *Data, unsigned int Size)
{
    /////////////////////////////////////////////////////////////////////////////////////////////////
    // return 0; for success
    // return 2; for No_ACK
    // return 4; for Arbitration
    /////////////////////////////////////////////////////////////////////////////////////////////////

    unsigned char status = 0;
    unsigned int i;
	
	
    for (i = 0; i < Size; i++)
    {
		if(is_SM768)
        	ddk768_swI2CWriteReg(IICAddr, (ByteAddr + i), *(Data + i));
		else
			swI2CWriteReg(IICAddr, (ByteAddr + i), *(Data + i));
	}
    return status;

}

static unsigned char IIC_Read(unsigned char IICAddr, unsigned char ByteAddr, unsigned char *Data, unsigned int Size)
{
    /////////////////////////////////////////////////////////////////////////////////////////////////
    // return 0; for success
    // return 2; for No_ACK
    // return 4; for Arbitration
    /////////////////////////////////////////////////////////////////////////////////////////////////

    unsigned char status = 0;
    unsigned int i;

    for (i = 0; i < Size; i++)
    {
    	if(is_SM768)
        	*(Data + i) = ddk768_swI2CReadReg(IICAddr,(ByteAddr + i));
		else
			*(Data + i) = swI2CReadReg(IICAddr,(ByteAddr + i));
	}	

    return status;

}
//--------------------------------------------------------------------------------------------------
//
// Hardware Interface
//

unsigned char EP952_Reg_Read(unsigned char ByteAddr, unsigned char *Data, unsigned int Size)
{
    return IIC_Read(IIC_EP952_Addr, ByteAddr, Data, Size);
}

unsigned char EP952_Reg_Write(unsigned char ByteAddr, unsigned char *Data, unsigned int Size)
{
    return IIC_Write(IIC_EP952_Addr, ByteAddr, Data, Size);
}

unsigned char EP952_Reg_Set_Bit(unsigned char ByteAddr, unsigned char BitMask)
{
    int result = 1;
	unsigned char Temp_Data[15] = {0};
    result = IIC_Read(IIC_EP952_Addr, ByteAddr, Temp_Data, 1);
    if(result == 0)
    {
        // Write back to Reg Reg_Addr
        Temp_Data[0] |= BitMask;

        return IIC_Write(IIC_EP952_Addr, ByteAddr, Temp_Data, 1);
    }
    else
    {
        return result;
    }
}

unsigned char EP952_Reg_Clear_Bit(unsigned char ByteAddr, unsigned char BitMask)
{
    int result = 1;
	unsigned char Temp_Data[15] = {0};
    result = IIC_Read(IIC_EP952_Addr, ByteAddr, Temp_Data, 1);
    if(result == 0)
    {
        // Write back to Reg Reg_Addr
        Temp_Data[0] &= ~BitMask;

        return IIC_Write(IIC_EP952_Addr, ByteAddr, Temp_Data, 1);
    }
    else
    {
        return result;
    }
}

//==================================================================================================
//
// Public Function Implementation
//

//--------------------------------------------------------------------------------------------------
// Hardware Interface

void EP952_IIC_Initial()
{
	unsigned char Temp_Data[15] = {0};
    IIC_EP952_Addr = 0x52;	  // EP952 slave address
    IIC_HDCPKey_Addr = 0xA8;  // HDCP Key (EEPROM) slave address


    // Initial Variables
    Temp_Data[0] = EP952_TX_PHY_Control_0__TERM_ON;
    EP952_Reg_Write(EP952_TX_PHY_Control_0, Temp_Data, 1);

    Temp_Data[0] = 0;
    EP952_Reg_Write(EP952_TX_PHY_Control_1, Temp_Data, 1);

	
}

void EP952_Info_Reset(void)
{
    int i;
	unsigned char Temp_Data[15] = {0};
    // Global date for HDMI Transmiter
    is_RSEN = 0;
    Cache_EP952_DE_Control = 0x03;

    // Initial Settings
    EP952_Reg_Set_Bit(EP952_Packet_Control, EP952_Packet_Control__VTX0);
    EP952_Reg_Set_Bit(EP952_General_Control_1, EP952_General_Control_1__INT_OD);

    //
    // Set Default AVI Info Frame
    //
    memset(Temp_Data, 0x00, 14);

    // Set AVI Info Frame to RGB
    Temp_Data[1] &= 0x60;
    Temp_Data[1] |= 0x00; // RGB

    // Set AVI Info Frame to 601
    Temp_Data[2] &= 0xC0;
    Temp_Data[2] |= 0x40;

    // Write AVI Info Frame
    Temp_Data[0] = 0;
    for(i=1; i<14; ++i) {
    	Temp_Data[0] += Temp_Data[i];
    }
    Temp_Data[0] = ~(Temp_Data[0] - 1);
    EP952_Reg_Write(EP952_AVI_Packet, Temp_Data, 14);

    //
    // Set Default ADO Info Frame
    //
    memset(Temp_Data, 0x00, 6);

    // Write ADO Info Frame
    Temp_Data[0] = 0;
    for(i=1; i<6; ++i) {
    	Temp_Data[0] += Temp_Data[i];
    }
    Temp_Data[0] = ~(Temp_Data[0] - 1);
    EP952_Reg_Write(EP952_ADO_Packet, Temp_Data, 6);

    //
    // Set Default CS Info Frame
    //
    memset(Temp_Data, 0x00, 5);
    	
    EP952_Reg_Write(EP952_Channel_Status, Temp_Data, 5);
/*
    //
    // clear Packet_1 Info Frame
    //
    Temp_Data[0] = 0;
    for(i=EP952_Data_Packet_Header; i<= 0x5F; i++) {	
    	EP952_Reg_Write(i, Temp_Data, 1);
    }
*/
}


// System flags
unsigned char is_Cap_dsel;
unsigned char is_Cap_HDMI;
unsigned char is_Cap_YCC444;
unsigned char is_Cap_YCC422;
unsigned char is_Forced_Output_RGB;

unsigned char is_Hot_Plug;
unsigned char is_Connected;
unsigned char is_ReceiverSense;
unsigned char ChkSum, ConnectionState;
unsigned char EP952_Debug = 0;

// System Data
unsigned char HP_ChangeCount;
unsigned char PowerUpCount;
TX_STATE TX_State;
VDO_PARAMS Video_Params;
ADO_PARAMS Audio_Params;
PEP952C_REGISTER_MAP pEP952C_Registers;

//------------------------------------
// Special for config

void HDMI_Tx_AMute_Enable(void)
{
	EP952_Reg_Set_Bit(EP952_Color_Space_Control, EP952_Color_Space_Control__AMUTE);
	EP952_Reg_Set_Bit(EP952_Pixel_Repetition_Control, EP952_Pixel_Repetition_Control__CTS_M);

	EP952_Reg_Clear_Bit(EP952_IIS_Control, EP952_IIS_Control__ADO_EN);		
	EP952_Reg_Clear_Bit(EP952_IIS_Control, EP952_IIS_Control__AUDIO_EN);	
}

void HDMI_Tx_AMute_Disable(void)
{
	EP952_Reg_Clear_Bit(EP952_Pixel_Repetition_Control, EP952_Pixel_Repetition_Control__CTS_M);
	EP952_Reg_Clear_Bit(EP952_Color_Space_Control, EP952_Color_Space_Control__AVMUTE);
	EP952_Reg_Clear_Bit(EP952_Color_Space_Control, EP952_Color_Space_Control__AMUTE);

	EP952_Reg_Set_Bit(EP952_IIS_Control, EP952_IIS_Control__ADO_EN);	
	EP952_Reg_Set_Bit(EP952_IIS_Control, EP952_IIS_Control__AUDIO_EN);
}

void HDMI_Tx_VMute_Enable(void)
{		
	EP952_Reg_Set_Bit(EP952_Color_Space_Control, EP952_Color_Space_Control__VMUTE);
}

void HDMI_Tx_VMute_Disable(void)
{
	EP952_Reg_Clear_Bit(EP952_Color_Space_Control, EP952_Color_Space_Control__AVMUTE);
	EP952_Reg_Clear_Bit(EP952_Color_Space_Control, EP952_Color_Space_Control__VMUTE);
}

//--------------------------------------------------------------------------------------------------
//
// HDMI Transmiter (EP952-Tx Implementation)
//

void HDMI_Tx_HDMI(void)
{
	EP952_Reg_Set_Bit(EP952_General_Control_4, EP952_General_Control_4__HDMI);
}

void HDMI_Tx_DVI(void)
{
	EP952_Reg_Clear_Bit(EP952_General_Control_4, EP952_General_Control_4__HDMI);
}

void HDMI_Tx_Power_Down(void)
{
	// Software power down
	EP952_Reg_Clear_Bit(EP952_General_Control_1, EP952_General_Control_1__PU);
}

void HDMI_Tx_Power_Up(void)
{
	// Software power up
	EP952_Reg_Set_Bit(EP952_General_Control_1, EP952_General_Control_1__PU);	
}

void HDMI_Tx_Mute_Enable(void)
{
	HDMI_Tx_AMute_Enable();
	HDMI_Tx_VMute_Enable();	
}

void HDMI_Tx_Mute_Disable(void)
{
	HDMI_Tx_VMute_Disable();	
	HDMI_Tx_AMute_Disable();
}

void HDMI_Tx_HDCP_Disable(void)
{
	EP952_Reg_Clear_Bit(EP952_General_Control_5, EP952_General_Control_5__ENC_EN);
}

void EP952_HDCP_Reset(void)
{
	pEP952C_Registers->System_Status |= EP_TX_System_Status__KEY_FAIL;
	pEP952C_Registers->System_Configuration |= EP_TX_System_Configuration__HDCP_DIS;
}

VDO_SETTINGS EP952_VDO_Settings[] = {
	//                   HVRes_Type,             		DE_Gen(DLY,CNT,TOP,LIN),                E_Sync, 				AR_PR,       Pix_Freq_Type,
	{  0,{       0,   0,   0,    0},{  0,   0,  0,   0},{0x00,  0,  0,  0,  0,   0},  0x00,                  PIX_FREQ_25175KHz}, // 0:
	// HDMI Mode
	{  1,{VNegHNeg, 800, 525,16666},{ 48, 640, 34, 480},{0x00, 12, 96, 10,  2,   0},  0x10,  PIX_FREQ_25175KHz}, // 1:    640 x  480p
	{  2,{VNegHNeg, 858, 525,16666},{ 60, 720, 31, 480},{0x00, 12, 62,  9,  6,   0},  0x10,  PIX_FREQ_27000KHz}, // 2:    720 x  480p  4:3
	{  3,{VNegHNeg, 858, 525,16666},{ 60, 720, 31, 480},{0x00, 12, 62,  9,  6,   0},  0x20,  PIX_FREQ_27000KHz}, // 3:    720 x  480p 16:9
	{  4,{VPosHPos,1650, 750,16666},{220,1280, 21, 720},{0x00,106, 40,  5,  5,   0},  0x20,  PIX_FREQ_74176KHz}, // 4:   1280 x  720p
	{  5,{VPosHPos,2200, 563,16666},{148,1920, 16, 540},{0x09, 84, 44,  2,  5,1100},  0x20,  PIX_FREQ_74176KHz}, // 5:   1920 x 1080i
	{  6,{VNegHNeg, 858, 262,16666},{ 57, 720, 16, 240},{0x09, 15, 62,  4,  3, 429},  0x15,  PIX_FREQ_27000KHz}, // 6:    720 x  480i, pix repl
	{  7,{VNegHNeg, 858, 262,16666},{ 57, 720, 16, 240},{0x09, 15, 62,  4,  3, 429},  0x25,  PIX_FREQ_27000KHz}, // 7:    720 x  480i, pix repl
	{  8,{VNegHNeg, 858, 262,16666},{ 57, 720, 16, 240},{0x00, 15, 62,  4,  3,   0},  0x15,  PIX_FREQ_27000KHz}, // 8:    720 x  240p, pix repl
	{  9,{VNegHNeg, 858, 262,16666},{ 57, 720, 16, 240},{0x00, 15, 62,  4,  3,   0},  0x25,  PIX_FREQ_27000KHz}, // 9:    720 x  240p, pix repl
	{ 10,{VNegHNeg,3432, 262,16666},{228,2880, 16, 240},{0x09, 72,248,  4,  3,1716},  0x10,  PIX_FREQ_54000KHz}, // 10:  2880 x  480i 4:3
	{ 11,{VNegHNeg,3432, 262,16666},{228,2880, 16, 240},{0x09, 72,248,  4,  3,1716},  0x20,  PIX_FREQ_54000KHz}, // 11:  2880 x  480i 16:9
	{ 12,{VNegHNeg,3432, 262,16666},{228,2880, 16, 240},{0x00, 72,248,  4,  3,   0},  0x10,  PIX_FREQ_54000KHz}, // 12:  2880 x  240p 4:3
	{ 13,{VNegHNeg,3432, 262,16666},{228,2880, 16, 240},{0x00, 72,248,  4,  3,   0},  0x20,  PIX_FREQ_54000KHz}, // 13:  2880 x  240p 16:9
	{ 14,{VNegHNeg,1716, 525,16666},{120,1440, 31, 480},{0x00, 28,124,  9,  6,   0},  0x10,  PIX_FREQ_54000KHz}, // 14:  1440 x  480p 4:3
	{ 15,{VNegHNeg,1716, 525,16666},{120,1440, 31, 480},{0x00, 28,124,  9,  6,   0},  0x20,  PIX_FREQ_54000KHz}, // 15:  1440 x  480p 16:9
	{ 16,{VPosHPos,2200,1125,16666},{148,1920, 37,1080},{0x00, 84, 44,  4,  5,   0},  0x20, PIX_FREQ_148500KHz}, // 16:  1920 x 1080p
	{ 17,{VNegHNeg, 864, 625,20000},{ 68, 720, 40, 576},{0x00,  8, 64,  5,  5,   0},  0x10,  PIX_FREQ_27000KHz}, // 17:   720 x  576p 4:3
	{ 18,{VNegHNeg, 864, 625,20000},{ 68, 720, 40, 576},{0x00,  8, 64,  5,  5,   0},  0x20,  PIX_FREQ_27000KHz}, // 18:   720 x  576p 16:9
	{ 19,{VPosHPos,1980, 750,20000},{220,1280, 21, 720},{0x00,436, 40,  5,  5,   0},  0x20,  PIX_FREQ_74250KHz}, // 19:  1280 x  720p, 50 Hz
	{ 20,{VPosHPos,2640, 563,20000},{148,1920, 16, 540},{0x09,524, 44,  2,  5,1320},  0x20,  PIX_FREQ_74250KHz}, // 20:  1920 x 1080i, 50 Hz
	{ 21,{VNegHNeg, 864, 313,20000},{ 69, 720, 20, 288},{0x09,  8, 63,  2,  3, 432},  0x15,  PIX_FREQ_27000KHz}, // 21:   720 x  576i, pix repl
	{ 22,{VNegHNeg, 864, 313,20000},{ 69, 720, 20, 288},{0x09,  8, 63,  2,  3, 432},  0x25,  PIX_FREQ_27000KHz}, // 22:   720 x  576i, pix repl
	{ 23,{VNegHNeg, 864, 313,20000},{ 69, 720, 20, 288},{0x00,  8, 63,  3,  3,   0},  0x15,  PIX_FREQ_27000KHz}, // 23:   720 x  288p, pix repl
	{ 24,{VNegHNeg, 864, 313,20000},{ 69, 720, 20, 288},{0x00,  8, 63,  3,  3,   0},  0x25,  PIX_FREQ_27000KHz}, // 24:   720 x  288p, pix repl
	{ 25,{VNegHNeg,3456, 313,20000},{276,2880, 20, 288},{0x09, 44,252,  2,  3,1728},  0x10,  PIX_FREQ_54000KHz}, // 25:  2880 x  576i
	{ 26,{VNegHNeg,3456, 313,20000},{276,2880, 20, 288},{0x09, 44,252,  2,  3,1728},  0x20,  PIX_FREQ_54000KHz}, // 26:  2880 x  576i
	{ 27,{VNegHNeg,3456, 313,20000},{276,2880, 20, 288},{0x00, 44,252,  3,  3,   0},  0x10,  PIX_FREQ_54000KHz}, // 27:  2880 x  288p
	{ 28,{VNegHNeg,3456, 313,20000},{276,2880, 20, 288},{0x00, 44,252,  3,  3,   0},  0x20,  PIX_FREQ_54000KHz}, // 28:  2880 x  288p
	{ 29,{VPosHNeg,1728, 625,20000},{136,1440, 40, 576},{0x00, 20,128,  5,  5,   0},  0x10,  PIX_FREQ_54000KHz}, // 29:  1440 x  576p
	{ 30,{VPosHNeg,1728, 625,20000},{136,1440, 40, 576},{0x00, 20,128,  5,  5,   0},  0x20,  PIX_FREQ_54000KHz}, // 30:  1440 x  576p
	{ 31,{VPosHPos,2640,1125,20000},{148,1920, 37,1080},{0x00,524, 44,  4,  5,   0},  0x20, PIX_FREQ_148500KHz}, // 31:  1920 x 1080p, 50 Hz
	{ 32,{VPosHPos,2750,1125,41666},{148,1920, 37,1080},{0x00,634, 44,  4,  5,   0},  0x20,  PIX_FREQ_74176KHz}, // 32:  1920 x 1080p
	{ 33,{VPosHPos,2640,1125,40000},{148,1920, 37,1080},{0x00,524, 44,  4,  5,   0},  0x20,  PIX_FREQ_74250KHz}, // 33:  1920 x 1080p, 25 Hz
	{ 34,{VPosHPos,2200,1125,33333},{148,1920, 37,1080},{0x00, 84, 44,  4,  5,   0},  0x20,  PIX_FREQ_74176KHz}, // 34:  1920 x 1080p

};

unsigned char EP952_VDO_Settings_Max = (sizeof(EP952_VDO_Settings)/sizeof(EP952_VDO_Settings[0]));

void HDMI_Tx_Video_Config(PVDO_PARAMS Params)
{
    int i;
	unsigned char Temp_Data[15] = {0};

    // Disable auto transmission AVI packet 
    EP952_Reg_Clear_Bit(EP952_IIS_Control, EP952_IIS_Control__AVI_EN);

    ////////////////////////////////////////////////////////
    // Video Interface

    // De_Skew
    EP952_Reg_Read(EP952_General_Control_3, Temp_Data, 1);
    Temp_Data[0] &= ~0xF0;
    Temp_Data[0] |= Params->Interface & 0xF0;
    EP952_Reg_Write(EP952_General_Control_3, Temp_Data, 1);	

    // input DSEL BSEL EDGE
    EP952_Reg_Read(EP952_General_Control_1, Temp_Data, 1);
    Temp_Data[0] &= ~0x0E;
    Temp_Data[0] |= Params->Interface & 0x0E;
	if(is_Cap_dsel)
    	Temp_Data[0] |= 0x08;   // DSEL = 1
    else
		Temp_Data[0] &= 0xf7;   // DSEL = 0
    EP952_Reg_Write(EP952_General_Control_1, Temp_Data, 1);	

    if(Params->Interface & 0x01) {
        EP952_Reg_Set_Bit(EP952_General_Control_4, EP952_General_Control_4__FMT12);
    }
    else {
        EP952_Reg_Clear_Bit(EP952_General_Control_4, EP952_General_Control_4__FMT12);
    }

    // update HVPol
    EP952_Reg_Read(EP952_General_Control_4, Temp_Data, 1);
    Params->HVPol = Temp_Data[0] & (EP952_DE_Control__VSO_POL | EP952_DE_Control__HSO_POL);	

    ////////////////////////////////////////////////////////
    // Sync Mode
    switch(Params->SyncMode) {
        default:
        case SYNCMODE_HVDE:
            // Disable E_SYNC
            EP952_Reg_Clear_Bit(EP952_General_Control_4, EP952_General_Control_4__E_SYNC);
            // Disable DE_GEN
            Cache_EP952_DE_Control &= ~EP952_DE_Control__DE_GEN;

            // Regular VSO_POL, HSO_POL
            if((Params->HVPol & VNegHPos) != (EP952_VDO_Settings[Params->VideoSettingIndex].HVRes_Type.HVPol & VNegHPos)) { // V
                Cache_EP952_DE_Control |= EP952_DE_Control__VSO_POL; // Invert
            }
            else {
                Cache_EP952_DE_Control &= ~EP952_DE_Control__VSO_POL;
            }
            if((Params->HVPol & VPosHNeg) != (EP952_VDO_Settings[Params->VideoSettingIndex].HVRes_Type.HVPol & VPosHNeg)) { // H
                Cache_EP952_DE_Control |= EP952_DE_Control__HSO_POL; // Invert
            }
            else {
                Cache_EP952_DE_Control &= ~EP952_DE_Control__HSO_POL;
            }
            break;

        case SYNCMODE_HV:
            // Disable E_SYNC
            EP952_Reg_Clear_Bit(EP952_General_Control_4, EP952_General_Control_4__E_SYNC);
            // Enable DE_GEN
            Cache_EP952_DE_Control |= EP952_DE_Control__DE_GEN;

            // Regular VSO_POL, HSO_POL
            if((Params->HVPol & VNegHPos) != (EP952_VDO_Settings[Params->VideoSettingIndex].HVRes_Type.HVPol & VNegHPos)) { // V
                Cache_EP952_DE_Control |= EP952_DE_Control__VSO_POL; // Invert
            }
            else {
                Cache_EP952_DE_Control &= ~EP952_DE_Control__VSO_POL;
            }
            if((Params->HVPol & VPosHNeg) != (EP952_VDO_Settings[Params->VideoSettingIndex].HVRes_Type.HVPol & VPosHNeg)) { // H
                Cache_EP952_DE_Control |= EP952_DE_Control__HSO_POL; // Invert
            }
            else {
                Cache_EP952_DE_Control &= ~EP952_DE_Control__HSO_POL;
            }

            // Set DE generation params
            if(Params->VideoSettingIndex < EP952_VDO_Settings_Max) {
                Cache_EP952_DE_Control &= ~0x03;

#ifdef Little_Endian
                Cache_EP952_DE_Control |= ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_DLY)[1];
                Temp_Data[0] = ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_DLY)[0];
                EP952_Reg_Write(EP952_DE_DLY, Temp_Data, 1);
#else	// Big Endian
                Cache_EP952_DE_Control |= ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_DLY)[0];
                Temp_Data[0] = ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_DLY)[1];
                EP952_Reg_Write(EP952_DE_DLY, Temp_Data, 1);
#endif

                Temp_Data[0] = ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_TOP)[0];
                EP952_Reg_Write(EP952_DE_TOP, Temp_Data, 1);

#ifdef Little_Endian
                Temp_Data[0] = ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_CNT)[0];
                Temp_Data[1] = ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_CNT)[1];
                EP952_Reg_Write(EP952_DE_CNT, Temp_Data, 2);
#else	// Big Endian
                Temp_Data[0] = ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_CNT)[1];
                Temp_Data[1] = ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_CNT)[0];
                EP952_Reg_Write(EP952_DE_CNT, Temp_Data, 2);
#endif


#ifdef Little_Endian
                Temp_Data[0] = ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_LIN)[1];
                Temp_Data[1] = ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_LIN)[0];
                EP952_Reg_Write(EP952_DE_LIN, Temp_Data, 2);
#else	// Big Endian
                Temp_Data[0] = ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_LIN)[1];
                Temp_Data[1] = ((unsigned char *)&EP952_VDO_Settings[Params->VideoSettingIndex].DE_Gen.DE_LIN)[0];
                EP952_Reg_Write(EP952_DE_LIN, Temp_Data, 2);
#endif

            }
            else {
                //DBG_printf(("ERROR: VideoCode overflow DE_GEN table\r\n"));
            }
            break;
    }
    EP952_Reg_Write(EP952_DE_Control, &Cache_EP952_DE_Control, 1);
	
    ////////////////////////////////////////////////////////
    // Pixel Repetition
    EP952_Reg_Read(EP952_Pixel_Repetition_Control, Temp_Data, 1);
    Temp_Data[0] &= ~EP952_Pixel_Repetition_Control__PR;
    if(Params->VideoSettingIndex < EP952_VDO_Settings_Max) {
        Temp_Data[0] |= EP952_VDO_Settings[Params->VideoSettingIndex].AR_PR & 0x03;
    }
    EP952_Reg_Write(EP952_Pixel_Repetition_Control, Temp_Data, 1);

    ////////////////////////////////////////////////////////
    // Color Space
    // Set to RGB
    EP952_Reg_Clear_Bit(EP952_General_Control_4, EP952_General_Control_4__YCC_IN | EP952_General_Control_4__422_IN);
    if(Params->VideoSettingIndex < EP952_VDO_Settings_IT_Start) { // CE Timing
        EP952_Reg_Clear_Bit(EP952_Color_Space_Control, EP952_Color_Space_Control__YCC_OUT | EP952_Color_Space_Control__422_OUT);
        EP952_Reg_Set_Bit(EP952_Color_Space_Control, EP952_Color_Space_Control__YCC_Range); // Output limit range RGB
    }
    else { // IT Timing
        EP952_Reg_Clear_Bit(EP952_Color_Space_Control, EP952_Color_Space_Control__YCC_OUT | EP952_Color_Space_Control__422_OUT | EP952_Color_Space_Control__YCC_Range);
    }

    // Color Space
    switch(Params->ColorSpace) {
        default:
        case COLORSPACE_601:
            // Set to 601
            EP952_Reg_Clear_Bit(EP952_Color_Space_Control, EP952_Color_Space_Control__COLOR);
            break;

        case COLORSPACE_709:
            // Set to 709
            EP952_Reg_Set_Bit(EP952_Color_Space_Control, EP952_Color_Space_Control__COLOR);
            break;
    }

    ///////////////////////////////////////////////////////////////////
    // AVI Info Frame
    //

    // clear AVI Info Frame
    memset(Temp_Data, 0x00, 14);

    // AVI InfoFrame Data Byte 1
    //switch(Params->FormatOut) {
    Temp_Data[1] |= 0x00; // AVI_Y1,Y0 = RGB
    Temp_Data[1] |= 0x10; // AVI_A0 = Active Format Information valid
	
    //SCAN
    switch(Params->SCAN) {
        default:
        case 0: 					
            Temp_Data[1] &= ~0x03;	// AVI_S1,S0 = No Data
            break;
        case 1: 					// AVI_S1,S0 = overscan
            Temp_Data[1] |= 0x01;	
            break;
        case 2: 					// AVI_S1,S0 = underscan
            Temp_Data[1] |= 0x02;	
            break;
    }

    // AVI InfoFrame Data Byte 2
    switch(Params->ColorSpace) {
        default:
        case COLORSPACE_601:
            Temp_Data[2] |= 0x40;	// AVI_C1,C0 = 601
            break;

        case COLORSPACE_709:
            Temp_Data[2] |= 0x80;	// AVI_C1,C0 = 709
            break;
    }

    if(Params->VideoSettingIndex < EP952_VDO_Settings_Max) {
        Temp_Data[2] |= EP952_VDO_Settings[Params->VideoSettingIndex].AR_PR & 0x30; // AVI_M1,M0 : Picture Aspect Ratio
    }
    Temp_Data[2] |= Params->AFARate & 0x0F;// AVI_R3~0: Active format Aspect Ratio

    // AVI InfoFrame Data Byte 3 is 0

    // AVI InfoFrame Data Byte 4 is VIC
    if(Params->VideoSettingIndex < EP952_VDO_Settings_IT_Start) {
        Temp_Data[4] |= EP952_VDO_Settings[Params->VideoSettingIndex].VideoCode;// AVI_VIC6~0 : Vedio Identification code
    }

    // AVI InfoFrame Data Byte 5
    if(Params->VideoSettingIndex < EP952_VDO_Settings_Max) {
        Temp_Data[5] |= (EP952_VDO_Settings[Params->VideoSettingIndex].AR_PR & 0x0C) >> 2;// AVI_PR3~0 : Pixel Repetition
    }

    // AVI InfoFrame Data Byte 0 is checksum
    Temp_Data[0] = 0x91;
        for(i=1; i<6; ++i) {
        Temp_Data[0] += Temp_Data[i];
    }
    Temp_Data[0] = ~(Temp_Data[0] - 1);	// checksum
	
    // Write AVI InfoFrame Data Byte
    EP952_Reg_Write(EP952_AVI_Packet, Temp_Data, 14);

    // Enable auto transmission AVI packet 
    EP952_Reg_Set_Bit(EP952_IIS_Control, EP952_IIS_Control__AVI_EN | EP952_IIS_Control__GC_EN);


}

//--------------------------------------------------------------------------------------------------------------------

void EP952Controller_Initial(PEP952C_REGISTER_MAP pEP952C_RegMap)
{		
    // Save the Logical Hardware Assignment
    pEP952C_Registers = pEP952C_RegMap;

    // Initial IIC address
    if(is_SM768)
    	ddk768_swI2CInit(30, 31);
	else
		swI2CInit(30, 31);
	
    EP952_IIC_Initial();

    // Software power down
    HDMI_Tx_Power_Down();

    // Enable Audio Mute and Video Mute
    HDMI_Tx_Mute_Enable();	

    // Reset Variables
    is_Cap_HDMI = 0;
    is_Cap_YCC444 = is_Cap_YCC422 = 0;
    is_Forced_Output_RGB = 0;
    is_Connected = 1;
    TX_State = TXS_Search_EDID;
    HP_ChangeCount = 0;
    PowerUpCount = 0;
    is_ReceiverSense = 0;

    // Reset all EP952 parameters
    pEP952C_Registers->System_Status = 0;
    pEP952C_Registers->EDID_ASFreq = 0;
    pEP952C_Registers->EDID_AChannel = 0;
    pEP952C_Registers->Video_change = 0;
    pEP952C_Registers->Audio_change = 0; 
    pEP952C_Registers->System_Configuration = 0;
    pEP952C_Registers->Video_Input_Format[1]= 0;
    //pEP952C_Registers->Video_Output_Format = 0;		// Auto select output 
    pEP952C_Registers->Video_Output_Format = 0x03;	    // Forced set RGB out

    // Disable HDCP
    HDMI_Tx_HDCP_Disable();

    // HDCP KEY reset
    EP952_HDCP_Reset();

    // Info Frame Reset
    EP952_Info_Reset();
}



#if 0
void EP_HDMI_DumpMessage(void)
{
    unsigned char temp_R = 0xFF, reg_addr = 0;

    ddk768_swI2CInit(30, 31);
    IIC_EP952_Addr = 0x52;	  // EP952 slave address
    IIC_HDCPKey_Addr = 0xA8;  // HDCP Key (EEPROM) slave address

    printk("[Samtest EP952 Register value]\r\n");
    printk("    -0 -1 -2 -3 -4 -5 -6 -7 -8 -9 -A -B -C -D -E -F\r\n");
    printk("    -----------------------------------------------");
    for(reg_addr=0; reg_addr<=0x88; reg_addr++)
    {
        EP952_Reg_Read(reg_addr, &temp_R, 1);

        if(reg_addr%16 == 0)
        {
            printk("\r\n%02x| ",((reg_addr/16)<<4));
        }
        printk("%02x ",temp_R);

    }
    printk("\r\n");
    printk("    -----------------------------------------------\r\n");
}

void EP_HDMI_DumpRegister(void)
{
    unsigned char temp_R = 0xFF, reg_addr = 0;

    ddk768_swI2CInit(30, 31);
    IIC_EP952_Addr = 0x52;	  // EP952 slave address
    IIC_HDCPKey_Addr = 0xA8;  // HDCP Key (EEPROM) slave address

    printk("    +++++");
    for(reg_addr=0x3A; reg_addr<=0x3D; reg_addr++)
    {
        EP952_Reg_Read(reg_addr, &temp_R, 1);

        printk("$%02X=%02X ", reg_addr, temp_R);

    }
    printk("    -----\r\n");
}
#endif
void EP_HDMI_Init(int chipID)
{
	if(chipID)
		is_SM768 = 1;
	else
		is_SM768 = 0;
	
    EP952Controller_Initial(&EP952C_Registers);
}

void EP952_Video_reg_set(void)
{
    // Mute Control
    HDMI_Tx_Mute_Enable();

    //if(pEP952C_Registers->Video_Input_Format[0] != 0)
    {
        // HDMI Mode
        if(!is_Cap_HDMI ) {
            HDMI_Tx_DVI();	// Set to DVI output mode (The Info Frame and Audio Packets would not be send)
            is_Forced_Output_RGB = 1;
        }
        else {
            HDMI_Tx_HDMI();	// Set to HDMI output mode
        }

    	///////////////////////////////////////////////////////////////////////
    	// Update Video Params
    	//
    	
    	// Video Interface
    	Video_Params.Interface = pEP952C_Registers->Video_Interface[0];
    	//Video_Params.Interface = 0x87
    		
    	// Video Timing
    	if(pEP952C_Registers->Video_Input_Format[0] < 35) {
    		Video_Params.VideoSettingIndex = pEP952C_Registers->Video_Input_Format[0];
    		//Video_Params.VideoSettingIndex = 0x16;
    	}
    	else{
    		Video_Params.VideoSettingIndex = 0;
    	}
    		
    	// Select Sync Mode
    	Video_Params.SyncMode = (SYNCMODE)((pEP952C_Registers->Video_Interface[1] & EP_TX_Video_Interface_Setting_1__SYNC) >> 2);
		//Video_Params.SyncMode = 0;
    		
    	// Select Color Space
    	switch(Video_Params.VideoSettingIndex) {
    		case  4: case  5: case 16: case 19: case 20: case 31: case 32: 
    		case 33: case 34: 													// HD Timing
    			Video_Params.ColorSpace = COLORSPACE_709;
    			break;
    		
    		default:
    			if(Video_Params.VideoSettingIndex) { 							// SD Timing
    				Video_Params.ColorSpace = COLORSPACE_601;
    			}
    			else {															// IT Timing
    				Video_Params.ColorSpace = COLORSPACE_709;
    			}
    			break;
    	}

    	// Forced Output RGB Format
    	if(pEP952C_Registers->Video_Output_Format == 0x03) {
    		is_Forced_Output_RGB = 1;
    	}
    		
    	// Set In and Output Color Format	
    	switch(pEP952C_Registers->Video_Interface[1] & EP_TX_Video_Interface_Setting_1__VIN_FMT) {
    		
    		default:
    		case EP_TX_Video_Interface_Setting_1__VIN_FMT__RGB:	 	// input is RGB
    			Video_Params.FormatIn = COLORFORMAT_RGB;
    			Video_Params.FormatOut = COLORFORMAT_RGB;
    			break;
    		
    		case EP_TX_Video_Interface_Setting_1__VIN_FMT__YCC444: 	// input is YCC444
    			Video_Params.FormatIn = COLORFORMAT_YCC444;
    			if(!is_Forced_Output_RGB && is_Cap_YCC444) {
    				Video_Params.FormatOut = COLORFORMAT_YCC444;
    			}
    			else {
    				Video_Params.FormatOut = COLORFORMAT_RGB;
    			}
    			break;
    	
    		case EP_TX_Video_Interface_Setting_1__VIN_FMT__YCC422: 	// inut is YCC422
    			Video_Params.FormatIn = COLORFORMAT_YCC422;
    			if(!is_Forced_Output_RGB && is_Cap_YCC422) {
    				Video_Params.FormatOut = COLORFORMAT_YCC422;
    			}
    			else {
    				Video_Params.FormatOut = COLORFORMAT_RGB;
    			}
    			break;
    	}
    	// AFAR
    	Video_Params.AFARate = AFAR_VideoCode;

    	// SCAN 		
    	Video_Params.SCAN = SCAN_NoData;

    	// Update EP952 Video Registers 
    	HDMI_Tx_Video_Config(&Video_Params);

    	// mute control
    	HDMI_Tx_Mute_Disable();
    }

    // clear flag
    pEP952C_Registers->Video_change = 0;

}

void EP_HDMI_Set_Video_Timing(unsigned int isHDMI, unsigned int dsel) //(logicalMode_t *pLogicalMode, unsigned int isHDMI)
{
    /////////////////////////////////////////////////////////////////////////
    //
    // need to select EP952 video input interface
    // detail description need reference to file " Video_Interface[0].jpg " and " Video_Interface[1].jpg "
    //
    /////////////////////////////////////////////////////////////////////////
    EP952C_Registers.Video_Interface[0] = 0x87;	// reference to Video_Interface[0].jpg;
    		
    EP952C_Registers.Video_Interface[1] = 0x00;	// reference to Video_Interface[1].jpg;


    /////////////////////////////////////////////////////////////////////////
    // need to select video timing, timing detail description as below
    //
    //  [Example]:
    // 		EP_HDMI_Set_Video_Timing(16); 	// 1080P 60Hz
    //
    // 		EP_HDMI_Set_Video_Timing(4); 	// 720P 60Hz
    //
    /////////////////////////////////////////////////////////////////////////
    EP952C_Registers.Video_Input_Format[0] = 16;		
    EP952C_Registers.Video_change = 1;					// Vedio setting change flag

    if (isHDMI)
        is_Cap_HDMI = 1;
    else
        is_Cap_HDMI = 0;

    if (dsel)
        is_Cap_dsel = 1;
    else
        is_Cap_dsel = 0;

    // power down Tx
    HDMI_Tx_Power_Down();

    // update EP952 register setting
    //EP952_Audio_reg_set();
    EP952_Video_reg_set();

    EP952_Debug = 1;
    	
    // Power Up Tx
    HDMI_Tx_Power_Up();

    TX_State = TXS_Stream;
}

void EP952_Register_Message(void)
{
	unsigned char temp[0x90] = {0};
    unsigned char reg_addr = 0;

    //ddk768_swI2CInit(30, 31);
    IIC_EP952_Addr = 0x52;	  // EP952 slave address
    IIC_HDCPKey_Addr = 0xA8;  // HDCP Key (EEPROM) slave address

    printk(KERN_INFO "[Samtest EP952 Register value]");
    printk(KERN_INFO " \t-0 -1 -2 -3 -4 -5 -6 -7 -8 -9 -A -B -C -D -E -F");
    printk(KERN_INFO " \t-----------------------------------------------");
    for(reg_addr=0; reg_addr<=0x88; reg_addr++)
    {
        EP952_Reg_Read(reg_addr, &temp[reg_addr], 1);
    }
	print_hex_dump(KERN_INFO, " \t", DUMP_PREFIX_NONE, 16, 1, temp, 0x90, false);
    printk(KERN_INFO " \t-----------------------------------------------");
}
