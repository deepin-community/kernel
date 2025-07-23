#include "ddk770_os.h"
#include "ddk770_reg.h"
#include "ddk770_power.h"
#include "ddk770_intr.h"
#include "ddk770_ddkdebug.h"
#include "ddk770_hwi2c.h"
#include "ddk770_hdmi_phy.h"
#include "ddk770_hdmi.h"
#include "ddk770_chip.h"
#include "ddk770_help.h"
#include <linux/delay.h>
#include <linux/timer.h>
#include "ddk770_hdmi_ddc.h"
#include "ddk770_hdmi_audio.h"
#include "ddk770_gpio.h"

int g_if_scrambling_lowR_HDMI[3] = { 0, 0, 0};
int g_scdc_present[3] = { 1, 1, 1};


static DEFINE_MUTEX(hdmi_mode_mutex);
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

void ddk770_HDMI_Write_Register(hdmi_index index, u32 offset, u32 value)
{
    u32 baseAddr;

    baseAddr = BASE_HDMI0 + (HDMI_BASE_OFFSET * index);

    pokeRegisterDWord(baseAddr + (offset << 2), value);
    return;
}

u32 ddk770_HDMI_Read_Register(hdmi_index index, u32 offset)
{
    u32 baseAddr;

    baseAddr = BASE_HDMI0 + (HDMI_BASE_OFFSET * index);

    return peekRegisterDWord(baseAddr + (offset << 2));
}

void ddk770_HDMI_write_mask(hdmi_index index, u32 addr, u32 mask, u32 data)
{
	u32 temp = 0;
	u32 shift = first_bit_set(mask);

	temp = ddk770_HDMI_Read_Register(index, addr);
	temp &= ~(mask);
	temp |= (mask & data << shift);
	ddk770_HDMI_Write_Register(index, addr, temp);
}

u32 ddk770_HDMI_read_mask(hdmi_index index, u32 addr, u32 mask)
{
	u32 shift = first_bit_set(mask);
	return ((ddk770_HDMI_Read_Register(index, addr) & mask) >> shift);
}


//----------------------------------------------------------------------
// Initial set
//------------------------------------ ----------------------------------
static void ddk770_HDMI_TX_Common_Init(
    hdmi_index index, u32 pixelClock, u32 freq)
{
    int data;

    printk("Initial HDMI Setting begin\n");

#if 1
	audio_Initialize(index);
	audio_Configure(index,pixelClock,freq);
#else
	ddk770_HDMI_Write_Register(index, 0x1025, 0x20);
    ddk770_HDMI_Write_Register(index, 0x1026, 0x33);    // 24bit 48kHz
    ddk770_HDMI_Write_Register(index, 0x3206, 0x02);
    ddk770_HDMI_Write_Register(index, 0x3205, 0x13);    // N = 3072  CTS = 222750
    ddk770_HDMI_Write_Register(index, 0x3204, 0xC6);
    ddk770_HDMI_Write_Register(index, 0x3203, 0xCC);
    ddk770_HDMI_Write_Register(index, 0x3202, 0x00);    // ncts_atomic_write = 0
    ddk770_HDMI_Write_Register(index, 0x3201, 0x50);
    ddk770_HDMI_Write_Register(index, 0x3200, 0x00);
#endif 


#if 1   //for hdcp1.4
    // HDCP1.4 config
    ddk770_HDMI_Write_Register(index, 0x5009, 0x1F);    // a_vidpolcfg
    ddk770_HDMI_Write_Register(index, 0x500a, 0x40);    // a_oesswcfg
    ddk770_HDMI_Write_Register(index, 0x5001, 0x04);    // a_hdcpcfg1
    ddk770_HDMI_Write_Register(index, 0x5000, 0x33);    // a_hdcpcfg0

    // // HDCP14 registers control and status
    ddk770_HDMI_Write_Register(index, 0x7904, 0x02);    // hdcp14reg_ctrl
    data = ddk770_HDMI_Read_Register(index, 0x7908);    // hdcp14reg_sts
    ddk770_HDMI_Write_Register(index, 0x7904, 0x03);    // hdcp14reg_ctrl
    data = ddk770_HDMI_Read_Register(index, 0x7908);    // hdcp14reg_sts
    ddk770_HDMI_Write_Register(index, 0x7904, 0x06);    // hdcp14reg_ctrl
    data = ddk770_HDMI_Read_Register(index, 0x7908);    // hdcp14reg_sts
    ddk770_HDMI_Write_Register(index, 0x7904, 0x00);    // hdcp14reg_ctrl
    data = ddk770_HDMI_Read_Register(index, 0x7908);    // hdcp14reg_sts

#else  //forhdcp2.2 
    // HDCP22 registers control and status
    ddk770_HDMI_Write_Register(index, 0x7904, 0x06);    // hdcp22reg_ctrl
    data = ddk770_HDMI_Read_Register(index, 0x7908);    // hdcp22reg_sts
    ddk770_HDMI_Write_Register(index, 0x7904, 0x07);    // hdcp22reg_ctrl
    data = ddk770_HDMI_Read_Register(index, 0x7908);    // hdcp22reg_sts


    data = ddk770_HDMI_Read_Register(index, 0x7904);    // hdcp22reg_sts
    ddk770_HDMI_Write_Register(index, 0x1000, 0xF8);    // hdcp22reg_ctrl
  
#endif

}


static void fc_video_PreambleFilter(hdmi_index index, u8 value, unsigned channel)
{
	
	if (channel == 0)
		ddk770_HDMI_Write_Register(index, (FC_CH0PREAM), value);
	else if (channel == 1)
		ddk770_HDMI_write_mask(index, FC_CH1PREAM, FC_CH1PREAM_CH1_PREAMBLE_FILTER_MASK, value);
	else if (channel == 2)
		ddk770_HDMI_write_mask(index, FC_CH2PREAM, FC_CH2PREAM_CH2_PREAMBLE_FILTER_MASK, value);
	else
		printk("invalid channel number: %d", channel);
}


static void fc_video_hdcp_keepout(hdmi_index index, u8 bit)
{

	ddk770_HDMI_write_mask(index, FC_INVIDCONF, FC_INVIDCONF_HDCP_KEEPOUT_MASK, bit);
}


__attribute__((unused)) static void fc_video_DviOrHdmi(hdmi_index index, u8 bit)
{
	/* 1: HDMI; 0: DVI */
	ddk770_HDMI_write_mask(index, FC_INVIDCONF, FC_INVIDCONF_DVI_MODEZ_MASK, bit);
}



//----------------------------------------------------------------------
// Cfg HDMI frame composer setting
//----------------------------------------------------------------------
static void ddk770_HDMI_TX_Common_CfgFrame(
    hdmi_index index,
    cea_parameter_t *cea_mode,
    bool isHDMI
)
{
    gColorFormat colorFormat = COLOR_RGB;
	u32 dataWr = 0x00;

    u32 vic = cea_mode->vic;
    u32 h_active = cea_mode->h_active;
    u32 h_blank = cea_mode->h_blank;
    u32 hsync_delay = cea_mode->hsync_delay;
    u32 hsync_width = cea_mode->hsync_width;
    u32 v_active = cea_mode->v_active;
    u32 v_blank = cea_mode->v_blank;
    u32 vsync_delay = cea_mode->vsync_delay;
    u32 vsync_width = cea_mode->vsync_width;
    u8 de_polarity = cea_mode->de_polarity;

    u8 hsync_polarity = cea_mode->hsync_polarity;
    u8 vsync_polarity = cea_mode->vsync_polarity;
    // logic [19:0] freq,
    u32 freq = cea_mode->freq;
    u8 vblank_osc = cea_mode->vblank_osc;
    u8 progressive_nI = cea_mode->progressive_nI;	
	int i;

    if (colorFormat == COLOR_YUV_420)
    {
        h_active = h_active / 2;
        h_blank = h_blank / 2;
        hsync_delay = hsync_delay / 2;
        hsync_width = hsync_width / 2;
    }

    dataWr = 0x00;
    dataWr |= (1 << 7);  ////Always due to HDCP and Scrambler
    dataWr |= (vsync_polarity << 6); // v polarity low
    dataWr |= (hsync_polarity << 5); // h polarity low
    dataWr |= (de_polarity << 4);    // de polarity high
    dataWr |= ((isHDMI == 1 ? 1 : 0) << 3); // HDMI mode/DVI Mode
    dataWr |= (vblank_osc << 1);
    dataWr &= ~(progressive_nI << 0);

	printk("HDMI Reg FC_INVIDCONF 0x1000 data=%x\n",dataWr);

    ddk770_HDMI_Write_Register(index, FC_INVIDCONF, dataWr);  // fc_invidcon vsync_in_polarity[6] hsync_in_polarity[5] 0 Low  de_in_polarity[4] 1 High

	fc_video_hdcp_keepout(index, 1);

	ddk770_HDMI_Write_Register(index, FC_INHACTIV1, h_active >> 8);         // fc_inhactiv1      //picture width[13:8]
    ddk770_HDMI_Write_Register(index, FC_INHACTIV0, h_active & 0xFF);       // fc_inhactiv0      //picture width[7:0]
    ddk770_HDMI_Write_Register(index, FC_INVACTIV1, v_active >> 8);         // fc_invactiv1      //picture height[13:8]
    ddk770_HDMI_Write_Register(index, FC_INVACTIV0, v_active & 0xFF);       // fc_invactiv0      //picture height[7:0]
    ddk770_HDMI_Write_Register(index, FC_INHBLANK1, h_blank >> 8);          // fc_inhblank1      //H blanking[12:8]  Between DE and DE
    ddk770_HDMI_Write_Register(index, FC_INHBLANK0, h_blank & 0xFF);        // fc_inhblank0      //H blanking[7:0]   Between DE and DE
    ddk770_HDMI_Write_Register(index, FC_INVBLANK, v_blank & 0xFF);        // fc_invblank0      //V blanking[7:0]   Between the last DE in a frame and the first DE of the next frame
    ddk770_HDMI_Write_Register(index, FC_HSYNCINDELAY1, hsync_delay >> 8);      // fc_hsyncindelay1  //Tail of DE to HSYNC[12:8]
    ddk770_HDMI_Write_Register(index, FC_HSYNCINDELAY0, hsync_delay & 0xFF);    // fc_hsyncindelay0  //Tail of DE to HSYNC[7:0]
    ddk770_HDMI_Write_Register(index, FC_VSYNCINDELAY, vsync_delay & 0xFF);    // fc_vsyncindelay0  //Tail of HSYNC to VSYNC[7:0]
    ddk770_HDMI_Write_Register(index, FC_HSYNCINWIDTH1, hsync_width >> 8);      // fc_hsyninwidth1   //Between HSYNC and HSYNC[9:8]
    ddk770_HDMI_Write_Register(index, FC_HSYNCINWIDTH0, hsync_width & 0xFF);    // fc_hsyninwidth0   //Between HSYNC and HSYNC[7:0]
    ddk770_HDMI_Write_Register(index, FC_VSYNCINWIDTH, vsync_width & 0xFF);    // fc_vsyninwidth0   //Between VSYNC and VSYNC[7:0]
    ddk770_HDMI_Write_Register(index, FC_INFREQ0, freq & 0xFF);           // fc_infreq0 0x100e & fc_infreq1 0x100f & fc_infreq2 0x1010
    ddk770_HDMI_Write_Register(index, FC_INFREQ1, (freq >> 8) & 0xFF);
    ddk770_HDMI_Write_Register(index, FC_INFREQ2, (freq >> 16) & 0xF);

	ddk770_HDMI_Write_Register(index, FC_CTRLDUR, 12);

	ddk770_HDMI_Write_Register(index, FC_EXCTRLDUR, 32);

	ddk770_HDMI_Write_Register(index, FC_EXCTRLSPAC, 1);

	for (i = 0; i < 3; i++)
		fc_video_PreambleFilter(index, (i + 1) * 11, i);


    ddk770_HDMI_Write_Register(index, FC_AVIVID,  vic); // vic


	
    switch (colorFormat)
    {
    case COLOR_RGB:
        ddk770_HDMI_Write_Register(index, 0x200, 0x01);
        break;

    case COLOR_YUV_444:
        ddk770_HDMI_Write_Register(index, 0x200, 0x09);
        break;

    case COLOR_YUV_422:
        ddk770_HDMI_Write_Register(index, 0x200, 0x16);
		
    	dataWr = ddk770_HDMI_Read_Register(index, 0x802);
  	    dataWr |= (1 << 2);
  	    ddk770_HDMI_Write_Register(index, 0x802, dataWr);
  	    dataWr = ddk770_HDMI_Read_Register(index, 0x804);
  	    dataWr |= (1 << 3) | (1 << 0);
  	    ddk770_HDMI_Write_Register(index, 0x804, dataWr);

        break;

    case COLOR_YUV_420:
        ddk770_HDMI_Write_Register(index, 0x200, 0x09);
        break;
    }


}

long ddk770_HDMI_Intr_Mute(hdmi_index index, u32 mute)
{
	if(mute)
		ddk770_HDMI_Write_Register(index, IH_MUTE, 0x3);
	else
		ddk770_HDMI_Write_Register(index, IH_MUTE, 0x0);
    
    return 0;
}


/*  
Return value
0x1 : HPD=1 RX Sense=0 //Need to setmode 
0x3 : HPD=1 RX Sense=1 //Need to setmode and notify OS
0x2 : HPD=0 RX Sense=1 //Nothing to do
0x0 : HPD=0 RX Sense=0 //Do plug out and notify OS

*/

long ddk770_HDMI_HPD_Detect(hdmi_index index) 
{
    u8 hpd, rxsense,value;

	hpd = phy_hot_plug_state(index);
	rxsense = phy_rx_sense_state(index);

	value = rxsense << 1 | hpd;

	if(value == 0x02){
		usleep_range(200000,201000);
		hpd = phy_hot_plug_state(index);
		rxsense = phy_rx_sense_state(index);
		value = rxsense << 1 | hpd;
	}
	return hpd;
}

int ddk770_HDMI_Read_EDID_Basic(hdmi_index index, u8 *pEDIDBuffer, u32 bufferSize, u16* pReadSize)
{
    int error = 0;

    const u8 header[] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};


	//i2cddc_clk_config(index, 2500, I2C_MIN_SS_SCL_LOW_TIME, I2C_MIN_SS_SCL_HIGH_TIME, I2C_MIN_FS_SCL_LOW_TIME, I2C_MIN_FS_SCL_HIGH_TIME);

#if 1
	//Reducing the HDMI IIC speed to 50khz to make the IIC more stable, just like DL
	ddk770_HDMI_Write_Register(index,0x7e0c,0xD8);
	ddk770_HDMI_Write_Register(index,0x7e0e,0xFE);

#else
	//Reducing the HDMI IIC speed to 25khz to make the IIC more stable, just like DL
	  ddk770_HDMI_Write_Register(index,0x7e0b,0x01);
	  ddk770_HDMI_Write_Register(index,0x7e0c,0xB0);
	  ddk770_HDMI_Write_Register(index,0x7e0d,0x01);
	  ddk770_HDMI_Write_Register(index,0x7e0e,0xFC);
#endif

    error = ddc_read(index, EDID_I2C_ADDR, EDID_I2C_SEGMENT_ADDR, 0, 0, bufferSize, pEDIDBuffer);
	if(error){
		printk("EDID basic read failed\n");
		return error;
	}

    error = memcmp(pEDIDBuffer, (u8 *) header, sizeof(header));
	if(error){
		printk("EDID header check failed\n");
		return error;
	}

    error = _edid_checksum(pEDIDBuffer);
	if(error){
		printk("EDID checksum failed\n");
		return error;
	}

    *pReadSize += bufferSize;

    return 0;
}

static int ddk770_HDMI_Read_EDID_Extension(hdmi_index index, u8 block, u8 *pEDIDBuffer, u32 bufferSize, u16* pReadSize)
{
    int error = 0;

    u8 start_pointer = block / 2; // pointer to segments of 256 bytes
	u8 start_address = ((block % 2) * 0x80); //offset in segment; first block 0-127; second 128-255

    pEDIDBuffer += (start_address + (start_pointer * 256));

    error = ddc_read(index, EDID_I2C_ADDR, EDID_I2C_SEGMENT_ADDR, start_pointer, start_address, bufferSize, pEDIDBuffer);
	if(error){
		printk("EDID extension read failed");
		return error;
	}

    error = _edid_checksum(pEDIDBuffer);
	if(error){
		printk("EDID checksum failed");
		return error;
	}

    *pReadSize += bufferSize;
    
    return 0;
}

static void ddk770_HDMI_Check_EDID_IIC_State(hdmi_index index)
{
    unsigned int gpioPin = peekRegisterDWord(GPIO_MUX);

    if (FIELD_VAL_GET(gpioPin, GPIO_MUX, I2C0) == GPIO_MUX_I2C0_ENABLE &&
        FIELD_VAL_GET(gpioPin, GPIO_MUX, I2C1) == GPIO_MUX_I2C1_ENABLE &&
        FIELD_VAL_GET(gpioPin, GPIO_MUX, I2C2) == GPIO_MUX_I2C2_ENABLE)
    {
        return;
    }
    else
    {
        ddk770_HDMI_Init(index);
    }
}

static int ddk770_HDMI_EDID_Block_Tag(const void *_block)
{
	const u8 *block = _block;

	return block[0];
}

static int ddk770_HDMI_EDID_Cea_Revision(const u8 *cea)
{
	/*
	 * FIXME is this correct for the DispID variant?
	 * The DispID spec doesn't really specify whether
	 * this is the revision of the CEA extension or
	 * the DispID CEA data block. And the only value
	 * given as an example is 0.
	 */
	return cea[1];
}

/*
 * References:
 * - CTA-861-H section 7.3.3 CTA Extension Version 3
 */
static int ddk770_HDMI_EDID_Cea_Db_Collection_Size(const u8 *cta)
{
	u8 d = cta[2];

	if (d < 4 || d > 127)
		return 0;

	return d - 4;
}

/* CTA-861-H section 7.4 CTA Data BLock Collection */
struct cea_db {
	u8 tag_length;
	u8 data[];
};

static int ddk770_HDMI_EDID_Cea_Db_tag(const struct cea_db *db)
{
	return db->tag_length >> 5;
}

static int ddk770_HDMI_EDID_Cea_Db_Payload_Len(const void *_db)
{
	/* FIXME: Transition to passing struct cea_db * everywhere. */
	const struct cea_db *db = _db;

	return db->tag_length & 0x1f;
}

static bool ddk770_HDMI_EDID_Cea_Db_Is_Extended_Tag(const struct cea_db *db, int tag)
{
	// cea_db_tag(db) == CTA_DB_EXTENDED_TAG == 7
	return ddk770_HDMI_EDID_Cea_Db_tag(db) == 7 &&
		ddk770_HDMI_EDID_Cea_Db_Payload_Len(db) >= 1 &&
		db->data[0] == tag;
}

static bool ddk770_HDMI_EDID_Cea_Db_Is_Hdmi_Forum_Eeodb(const void *db)
{
	// CTA_EXT_DB_HF_EEODB == 0x78
	return ddk770_HDMI_EDID_Cea_Db_Is_Extended_Tag(db, 0x78) &&
		ddk770_HDMI_EDID_Cea_Db_Payload_Len(db) >= 2;
}

/*
 * Get the HF-EEODB override extension block count from EDID.
 *
 * The passed in EDID may be partially read, as long as it has at least two
 * blocks (base block and one extension block) if EDID extension count is > 0.
 *
 * Note that this is *not* how you should parse CTA Data Blocks in general; this
 * is only to handle partially read EDIDs. Normally, use the CTA Data Block
 * iterators instead.
 *
 * References:
 * - HDMI 2.1 section 10.3.6 HDMI Forum EDID Extension Override Data Block
 */
static int ddk770_HDMI_EDID_Hfeeodb_Block_Count(u8 *pEDIDBuffer)
{
	const u8 *cta;

	/* No extensions according to base block, no HF-EEODB. */
	if (!pEDIDBuffer[126])
		return 0;

	/* HF-EEODB is always in the first EDID extension block only */
	cta = &pEDIDBuffer[EDID_LENGTH];
	if (ddk770_HDMI_EDID_Block_Tag(cta) != 0x02 || ddk770_HDMI_EDID_Cea_Revision(cta) < 3)	// CEA_EXT = 0x02
		return 0;

	/* Need to have the data block collection, and at least 3 bytes. */
	if (ddk770_HDMI_EDID_Cea_Db_Collection_Size(cta) < 3)
		return 0;

	/*
	 * Sinks that include the HF-EEODB in their E-EDID shall include one and
	 * only one instance of the HF-EEODB in the E-EDID, occupying bytes 4
	 * through 6 of Block 1 of the E-EDID.
	 */
	if (!ddk770_HDMI_EDID_Cea_Db_Is_Hdmi_Forum_Eeodb(&cta[4]))
		return 0;
	
	return cta[4 + 2];
}

void ddk770_HDMI_set_SCDC(hdmi_index index, u8 *pEDIDBuffer)
{
	// Search for HF-VSDB if HDMI support scrambling under 340M TMDS Clk.
	int i;
	u8 scrambling_block_lowR;
	u16 offset_block;

	scrambling_block_lowR = pEDIDBuffer[126] < 3 ? 1 : 2;
	offset_block = scrambling_block_lowR * EDID_LENGTH;
	g_if_scrambling_lowR_HDMI[index] = 0;
	g_scdc_present[index] = 0;
	for (i = 4; i < 4 + pEDIDBuffer[offset_block + 2];) {
		int tag_code = pEDIDBuffer[offset_block + i] >> 5;
		int length = pEDIDBuffer[offset_block + i] & 0x1F;

		// printk("i %x pEDIDBuffer[offset_block + i] %x tag_code %x length %x\n", i, pEDIDBuffer[offset_block + i], tag_code, length);

		// Check if the block is HF-VSDB (Vendor Specific Data Block with HDMI identifier)
		/*
		* All HDMI 2.0 monitors must support scrambling at rates > 340 MHz.
		* And as per the spec, three factors confirm this:
		* * Availability of a HF-VSDB block in EDID (check)
		* * Non zero Max_TMDS_Char_Rate filed in HF-VSDB (let's check)
		* * SCDC support available (let's check)
		* Lets check it out.
		*/
		if (tag_code == 0x03 && length >= 6 &&
		    pEDIDBuffer[offset_block + i + 1] == 0xD8 &&
		    pEDIDBuffer[offset_block + i + 2] == 0x5D &&
		    pEDIDBuffer[offset_block + i + 3] == 0xC4) {
			int scdc_present = pEDIDBuffer[offset_block + i + 6] >>
					   7;
			if (scdc_present)
				g_scdc_present[index] = 1;
			if (pEDIDBuffer[offset_block + i + 5]) {
				unsigned char byte_scramble =
					pEDIDBuffer[offset_block + i + 6];
				if (g_scdc_present[index])
					g_if_scrambling_lowR_HDMI[index] =
						byte_scramble & (0x1 << 3) ? 1 :
									     0;
			}
		}
		i += (length + 1);
	}
}

long ddk770_HDMI_Read_EDID(hdmi_index index, u8 *pEDIDBuffer, u16* bufferSize)
{

    int ret = 0, edid_tries = 3;
    int num_blocks = 0;//, invalid_blocks = 0;
	int i;
    *bufferSize = 0;

	g_scdc_present[index] = 1;

    ddk770_HDMI_Check_EDID_IIC_State(index);

	if(!ddk770_HDMI_HPD_Detect(index))
	{
		//not plug-in
		// memset(pEDIDBuffer, 0, *bufferSize);
		return -1;
	}

    //DelayMs(600);
    do
    {

        ret = ddk770_HDMI_Read_EDID_Basic(index, pEDIDBuffer, EDID_LENGTH, bufferSize);
        if (ret)
        {
        	ddk770_i2c_reset_busclear(index);
            continue;
        }
        break;
    } while (edid_tries--);

    if (ret)
    {
        return ret;
    }
    
	num_blocks = pEDIDBuffer[126];

    // 1 block per 128 bytes
    if (num_blocks)
    for (i = 0; i < num_blocks; i++)    
    {
        edid_tries = 3;
        do
        {
            ret = ddk770_HDMI_Read_EDID_Extension(index, i + 1, pEDIDBuffer, EDID_LENGTH, bufferSize);
            if (ret)
            {
            	ddk770_i2c_reset_busclear(index);
                continue;
            }
			if (i == 0)
			{
				int eeodb = ddk770_HDMI_EDID_Hfeeodb_Block_Count(pEDIDBuffer);

				if (eeodb > num_blocks)
				{
					num_blocks = eeodb;
				}
			}

            break;
        } while (edid_tries--);
    }

	ddk770_HDMI_set_SCDC(index, pEDIDBuffer);


    // Search for HF-VSDB if HDMI support scrambling under 340M TMDS Clk.

    // scrambling_block_lowR = pEDIDBuffer[126] < 3 ? 1 : 2;
    // offset_block = scrambling_block_lowR * EDID_LENGTH;
    // g_if_scrambling_lowR_HDMI[index] = 0;
	// g_scdc_present[index] = 0;
    // for (i = 4; i < 4 + pEDIDBuffer[offset_block + 2]; )
    // {
    //     int tag_code = pEDIDBuffer[offset_block + i] >> 5;
    //     int length = pEDIDBuffer[offset_block + i] & 0x1F;
		
    //     // printk("i %x pEDIDBuffer[offset_block + i] %x tag_code %x length %x\n", i, pEDIDBuffer[offset_block + i], tag_code, length);

    //     // Check if the block is HF-VSDB (Vendor Specific Data Block with HDMI identifier)
	// 	/*
	// 	* All HDMI 2.0 monitors must support scrambling at rates > 340 MHz.
	// 	* And as per the spec, three factors confirm this:
	// 	* * Availability of a HF-VSDB block in EDID (check)
	// 	* * Non zero Max_TMDS_Char_Rate filed in HF-VSDB (let's check)
	// 	* * SCDC support available (let's check)
	// 	* Lets check it out.
	// 	*/
	// 	if (tag_code == 0x03 && length >= 6 && pEDIDBuffer[offset_block + i + 1] == 0xD8 && pEDIDBuffer[offset_block + i + 2] == 0x5D && pEDIDBuffer[offset_block + i + 3] == 0xC4)
	// 	{
	// 		int scdc_present = pEDIDBuffer[offset_block + i + 6] >> 7;
	// 		if (scdc_present)
	// 			g_scdc_present[index] = 1;
	// 		if (pEDIDBuffer[offset_block + i + 5])
	// 		{
	// 			unsigned char byte_scramble = pEDIDBuffer[offset_block + i + 6];
	// 			if (g_scdc_present[index])
	// 				g_if_scrambling_lowR_HDMI[index] = byte_scramble & (0x1 << 3) ? 1 : 0;

	// 		}
	// 	}
	// 	i += (length + 1);
    // }

    return ret;
}

void ddk770_HDMI_Set_Channel(hdmi_index index, disp_control_t dc)
{
    disp_output_t disp_index;
    if (index == INDEX_HDMI0)
    {
        disp_index = HDMI0;
    }else if (index == INDEX_HDMI1)
    {
        disp_index = HDMI1;
    }else if (index == INDEX_HDMI2)
    {
        disp_index = HDMI2;
    }
    
	setDCMUX(disp_index, dc);
}

void ddk770_HDMI_Clear_Channel(hdmi_index index)
{
    disp_output_t disp_index;
    if (index == INDEX_HDMI0)
    {
        disp_index = HDMI0;
    }else if (index == INDEX_HDMI1)
    {
        disp_index = HDMI1;
    }else if (index == INDEX_HDMI2)
    {
        disp_index = HDMI2;
    }
    
	ClearDCMUX(disp_index);
}

unsigned char ddk770_HDMI_Get_Channel(hdmi_index index)
{
    disp_output_t disp_index;
    if (index == INDEX_HDMI0)
    {
        disp_index = HDMI0;
    }else if (index == INDEX_HDMI1)
    {
        disp_index = HDMI1;
    }else if (index == INDEX_HDMI2)
    {
        disp_index = HDMI2;
    }
    
	return GetDCMUX(disp_index);
}

void ddk770_i2c_reset_busclear(hdmi_index index)
{
	i2c_reset(index);
	i2c_bus_clear(index);

}



static void _EnableAvmute(hdmi_index index, u8 bit)
{
	ddk770_HDMI_write_mask(index, A_HDCPCFG0, A_HDCPCFG0_AVMUTE_MASK, bit);
}

static void hdcp_av_mute(hdmi_index index, int enable)
{

	_EnableAvmute(index,
			(enable == 1) ? 1 : 0);
}

static void packets_AvMute(hdmi_index index, u8 enable)
{

	ddk770_HDMI_write_mask(index, FC_GCP, FC_GCP_SET_AVMUTE_MASK, (enable ? 1 : 0));
	ddk770_HDMI_write_mask(index, FC_GCP, FC_GCP_CLEAR_AVMUTE_MASK, (enable ? 0 : 1));
}

static void api_avmute(hdmi_index index, int enable)
{
	packets_AvMute(index, enable);
	hdcp_av_mute(index, enable);
}

static void fc_force_audio(hdmi_index index, u8 bit)
{
	ddk770_HDMI_write_mask(index, FC_DBGFORCE, FC_DBGFORCE_FORCEAUDIO_MASK, bit);
}


static void fc_force_video(hdmi_index index, u8 bit)
{
	
	/* avoid glitches */
	if (bit != 0) {
		ddk770_HDMI_Write_Register(index, FC_DBGTMDS2, 0x00);	/* R */
		ddk770_HDMI_Write_Register(index, FC_DBGTMDS1, 0x00);	/* G */
		ddk770_HDMI_Write_Register(index, FC_DBGTMDS0, 0xFF);	/* B */
		ddk770_HDMI_write_mask(index, FC_DBGFORCE, FC_DBGFORCE_FORCEVIDEO_MASK, 1);
	} else {
		ddk770_HDMI_write_mask(index, FC_DBGFORCE, FC_DBGFORCE_FORCEVIDEO_MASK, 0);
		ddk770_HDMI_Write_Register(index, FC_DBGTMDS2, 0x00);	/* R */
		ddk770_HDMI_Write_Register(index, FC_DBGTMDS1, 0x00);	/* G */
		ddk770_HDMI_Write_Register(index, FC_DBGTMDS0, 0x00);	/* B */
	}
}

static void fc_force_output(hdmi_index index, int enable)
{
	fc_force_audio(index, 0);
	fc_force_video(index, (u8)enable);
}


static void mc_disable_all_clocks(hdmi_index index)
{
	ddk770_HDMI_Write_Register(index, MC_CLKDIS, 0xff);
}

static void mc_enable_all_clocks(hdmi_index index)
{
	ddk770_HDMI_Write_Register(index, MC_CLKDIS, 0x00);
}



int ddk770_HDMI_Standby(hdmi_index index)
{
	mc_disable_all_clocks(index);
	phy_standby(index);

	return 1;
}






void ddk770_HDMI_Init(hdmi_index index)
{
    unsigned int gpioPin;

	//Set GPIO to HDMI I2C
    gpioPin = peekRegisterDWord(GPIO_MUX);

    pokeRegisterDWord(GPIO_MUX,                                            
        FIELD_SET(gpioPin, GPIO_MUX, I2C2, ENABLE) |                          
        FIELD_SET(gpioPin, GPIO_MUX, I2C1, ENABLE) |                           
        FIELD_SET(gpioPin, GPIO_MUX, I2C0, ENABLE));


	irq_mask_all(index);  //except PHY

	irq_hpd_sense_enable(index);
		
	//Always due to HDCP and Scrambler
	fc_video_hdcp_keepout(index, 1);

	mc_disable_all_clocks(index);

	phy_initialize(index);
	
	 ddk770_HDMI_Clear_Intr_State(index);

	// disable blue screen transmission after turning on all necessary blocks (e.g. HDCP)
	fc_force_output(index, 1);

	// unmask interrupts.Can't enable interrupt.
//	HDMI_Intr_Mute(index,0);
	ddk770_HDMI_phy_interrupt_mask(index, PHY_MASK0_TX_PHY_LOCK_MASK|
				  PHY_MASK0_RX_SENSE_0_MASK |
				  PHY_MASK0_RX_SENSE_1_MASK |
				  PHY_MASK0_RX_SENSE_2_MASK |
				  PHY_MASK0_RX_SENSE_3_MASK);
	ddk770_HDMI_phy_interrupt_unmask(index, PHY_MASK0_HPD_MASK);


}


long ddk770_HDMI_Set_Mode(hdmi_index index, logicalMode_t *pLogicalMode, mode_parameter_t *pModeParam, bool isHDMI)
{
    cea_parameter_t cea_mode = {0};
    u8 scdc_val;
    long ret =0;
	ret = Get_CEA_Mode(pLogicalMode,&cea_mode, pModeParam, 0);
	if(ret < 0)
	{
		return ret;
    }

	mutex_lock(&hdmi_mode_mutex);
	phy_standby(index);
    api_avmute(index, 1);
	ddk770_HDMI_Intr_Mute(index,1);

    printk("HDMI %d Set Mode\n",index);
	
	fc_force_output(index, 1);
		
	if (cea_mode.tmds_clk > 340000)
	{
		scdc_val = 0x1;
		scdc_write(index, SCDC_SINK_VER, 1, &scdc_val);
		scrambling(index, 1);
		scdc_tmds_high_enable_flag(index, 1);
		printk("Enabling Scrambling and Enable TMDS High.\n");
	}
	else if (g_if_scrambling_lowR_HDMI[index])
	{ //<340MHz, but has scrambling_lowR bit
		scdc_val = 0x1;
		scdc_write(index, SCDC_SINK_VER, 1, &scdc_val);
		scrambling(index, 1);
		scdc_tmds_high_enable_flag(index, 0);
		printk("Enabling Scrambling and Disable TMDS High.\n");
	}
	else if (g_scdc_present[index])
	{ // <340MHz	
		scrambling(index, 0);
		scdc_tmds_high_enable_flag(index, 0);
		printk("Disable Scrambling.\n");
	}else{
		printk("Just Disable Scrambling on TX.\n");
		scrambling_Enable(index, 0);
	}

	//Video

	ddk770_HDMI_TX_Common_CfgFrame(index,&cea_mode,isHDMI);

	//Audio and HDCP

	ddk770_HDMI_TX_Common_Init(index,cea_mode.tmds_clk, SAMPLE_RATE);

	//Enable All clocks

	mc_enable_all_clocks(index);

	usleep_range(10000,11000);

	//HDMI PHY Set
	
    ret = ddk770_HDMI_PHY_Set_Mode(index,cea_mode.tmds_clk);
                

	// Disable blue screen transmission after turning on all necessary blocks (e.g. HDCP)
	fc_force_output(index, 0);
	
	ddk770_HDMI_Intr_Mute(index,0);
    api_avmute(index,0);
	mutex_unlock(&hdmi_mode_mutex);
    return ret;

}





void ddk770_HDMI_Enable_Output(hdmi_index index)
{
	 api_avmute(index,0);
}

void ddk770_HDMI_Disable_Output(hdmi_index index)
{
	 api_avmute(index,1);
}



void ddk770_HDMI_Audio_Mute(hdmi_index index)
{
	fc_audio_mute(index);
}


void ddk770_HDMI_Audio_Unmute(hdmi_index index)
{
	fc_audio_unmute(index);
}


 
//-----------------------------------------------------------------------------
// HDMI Interrupt functions
//-----------------------------------------------------------------------------
typedef struct _hdmi_interrupt_t
{
    struct _hdmi_interrupt_t *next;
    void (*handler)(void);
}
hdmi_interrupt_t;

static hdmi_interrupt_t *g_pHdmi0IntHandlers = ((hdmi_interrupt_t *)0);
static hdmi_interrupt_t *g_pHdmi1IntHandlers = ((hdmi_interrupt_t *)0);
static hdmi_interrupt_t *g_pHdmi2IntHandlers = ((hdmi_interrupt_t *)0);



static void irq_clear_hpd_source(hdmi_index index)
{
	ddk770_HDMI_Write_Register(index, IH_PHY_STAT0,0xff);	
}


void irq_mask_all(hdmi_index index)
{
	ddk770_HDMI_Intr_Mute(index,1); /* disable interrupts */

	ddk770_HDMI_Write_Register(index, IH_MUTE_FC_STAT0,  0xff);
	ddk770_HDMI_Write_Register(index, IH_MUTE_FC_STAT1,  0xff);
	ddk770_HDMI_Write_Register(index, IH_MUTE_FC_STAT2,  0xff);
	ddk770_HDMI_Write_Register(index, IH_MUTE_AS_STAT0,  0xff);
	ddk770_HDMI_Write_Register(index, IH_MUTE_I2CM_STAT0,  0xff);
	ddk770_HDMI_Write_Register(index, IH_MUTE_CEC_STAT0,  0xff);
	ddk770_HDMI_Write_Register(index, IH_MUTE_VP_STAT0,  0xff);
	ddk770_HDMI_Write_Register(index, IH_MUTE_I2CMPHY_STAT0,  0xff);
	ddk770_HDMI_Write_Register(index, IH_MUTE_AHBDMAAUD_STAT0,  0xff);
	ddk770_HDMI_Write_Register(index, IH_MUTE_PHY_STAT0,  0xff);
}






void ddk770_HDMI_Clear_Intr_State(hdmi_index index)
{

	irq_clear_hpd_source(index);
}


/* HDMI Interrupt Service Routine */
__attribute__((unused)) static void hdmi0ISR(
    unsigned long status)
{
    hdmi_interrupt_t *pfnHandler;
    u32 decode = 0;
    u32 phy_decode = 0;


	if (FIELD_VAL_GET(status, INT_STATUS, HDMI0) == INT_STATUS_HDMI0_ACTIVE)
	{
	    decode = read_interrupt_decode(INDEX_HDMI0);
		
	    if (decode_is_phy(decode))
	    {
	      phy_decode = ddk770_HDMI_Read_Register(INDEX_HDMI0, IH_PHY_STAT0);
	   
		  if (decode_is_phy_hpd(phy_decode)) 
		  {
		        // What we need to do in ISR
		        printk("We are in hdmi0 hpd ISR\n");

		            /* Walk all registered handlers for handlers that support this interrupt status */
		            for (pfnHandler = g_pHdmi0IntHandlers; pfnHandler != ((hdmi_interrupt_t *)0); pfnHandler = pfnHandler->next)
		                pfnHandler->handler();
		      

		       phy_hot_plug_detected(INDEX_HDMI0);
		  }
	    }
			/* Clear all the HDMI Interrupt */ 
		     ddk770_HDMI_Clear_Intr_State(INDEX_HDMI0); 
	}


}

/* HDMI Interrupt Service Routine */
__attribute__((unused)) static void hdmi1ISR(
    unsigned long status
)
{
    hdmi_interrupt_t *pfnHandler;
    u32 decode = 0;
    u32 phy_decode = 0;

	if (FIELD_VAL_GET(status, INT_STATUS, HDMI1) == INT_STATUS_HDMI1_ACTIVE)
	{
	    decode = read_interrupt_decode(INDEX_HDMI1);
		
	    if (decode_is_phy(decode))
	    {
	      phy_decode = ddk770_HDMI_Read_Register(INDEX_HDMI1, IH_PHY_STAT0);
	   
		  if (decode_is_phy_hpd(phy_decode)) 
		  {
		        // What we need to do in ISR
		        printk("We are in hdmi1 hpd ISR\n");

		            /* Walk all registered handlers for handlers that support this interrupt status */
		            for (pfnHandler = g_pHdmi1IntHandlers; pfnHandler != ((hdmi_interrupt_t *)0); pfnHandler = pfnHandler->next)
		                pfnHandler->handler();
		      

		        phy_hot_plug_detected(INDEX_HDMI1);
		  }
	    }
			/* Clear all the HDMI Interrupt */ 
		    ddk770_HDMI_Clear_Intr_State(INDEX_HDMI1); 
	}

}


/* HDMI Interrupt Service Routine */
__attribute__((unused)) static void hdmi2ISR(
    unsigned long status
)
{
    hdmi_interrupt_t *pfnHandler;
    u32 decode = 0;
    u32 phy_decode = 0;

	if (FIELD_VAL_GET(status, INT_STATUS, HDMI2) == INT_STATUS_HDMI2_ACTIVE)
	{
	    decode = read_interrupt_decode(INDEX_HDMI2);
		
	    if (decode_is_phy(decode))
	    {
	      phy_decode = ddk770_HDMI_Read_Register(INDEX_HDMI2, IH_PHY_STAT0);
	   
		  if (decode_is_phy_hpd(phy_decode)) 
		  {
		        // What we need to do in ISR
		        printk("We are in hdmi2 hpd ISR\n");

		            /* Walk all registered handlers for handlers that support this interrupt status */
		            for (pfnHandler = g_pHdmi2IntHandlers; pfnHandler != ((hdmi_interrupt_t *)0); pfnHandler = pfnHandler->next)
		                pfnHandler->handler();
		      

		       phy_hot_plug_detected(INDEX_HDMI2);
		  }
	    }
			/* Clear all the HDMI Interrupt */ 
		    ddk770_HDMI_Clear_Intr_State(INDEX_HDMI2); 
	}

}


int hdmiISR(
    hdmi_index index)
{
	int intStatus;
    u32 decode = 0;
    u32 phy_decode = 0;
	int ret = 0;
	int bit_value = 0;
	intStatus = peekRegisterDWord(INT_STATUS);
	if(index == (hdmi_index)HDMI0)
		bit_value = (intStatus >>11)&1;
	else if(index == (hdmi_index)HDMI1)
		bit_value = (intStatus >>13)&1;
	else if(index == (hdmi_index)HDMI2)
		bit_value = (intStatus >>15)&1;
	if (bit_value == INT_STATUS_HDMI0_ACTIVE)
	{
	    decode = read_interrupt_decode(index);
		
	    if (decode_is_phy(decode))
	    {
	      phy_decode = ddk770_HDMI_Read_Register(index, IH_PHY_STAT0);
	   
		  if (decode_is_phy_hpd(phy_decode)) 
		  {
		        // What we need to do in ISR
		        printk("We are in hdmi%d hpd ISR\n",index);
		      	ret = HDMI_INT_HPD;
		       	phy_hot_plug_detected(index);
			
		  }else
		  	ret = HDMI_INT_NOT_HPD;
	    }else
			ret = HDMI_INT_NOT_HPD;
			/* Clear all the HDMI Interrupt */ 
		ddk770_HDMI_Clear_Intr_State(index); 
	}
	return ret;

}


void ddk770_HDMI_interrupt_enable(hdmi_index index, unsigned int enable)
{
	unsigned int value;
	value = peekRegisterDWord(INT_MASK);

	if(enable)
	{
		ddk770_HDMI_Intr_Mute(index,0);

		if(index == INDEX_HDMI0)
			value = FIELD_SET(value, INT_MASK, HDMI0, ENABLE);
		else if(index == INDEX_HDMI1)
			value = FIELD_SET(value, INT_MASK, HDMI1, ENABLE);
		else
			value = FIELD_SET(value, INT_MASK, HDMI2, ENABLE);
			
		pokeRegisterDWord(INT_MASK, value);


	}else{
		ddk770_HDMI_Intr_Mute(index,1);

		if(index == INDEX_HDMI0)
			value = FIELD_SET(value, INT_MASK, HDMI0, DISABLE);
		else if(index == INDEX_HDMI1)
			value = FIELD_SET(value, INT_MASK, HDMI1, DISABLE);
		else
			value = FIELD_SET(value, INT_MASK, HDMI2, DISABLE);
			
		pokeRegisterDWord(INT_MASK, value);

	}
	
	
}

#if USE_I2C_ADAPTER
static int ddk770_hdmi_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[],
			   int num)
{	
	struct smi_connector *connector = i2c_get_adapdata(adap);
	int i, index;
	int ret = 0;
	u8 addr = msgs[0].addr;

	if (addr == DDC_CI_ADDR)
		/*
		 * The internal I2C controller does not support the multi-byte
		 * read and write operations needed for DDC/CI.
		 * TOFIX: Blacklist the DDC/CI address until we filter out
		 * unsupported I2C operations.
		 */
		return -EOPNOTSUPP;

	for (i = 0; i < num; i++) {
		if (msgs[i].len == 0) {
			pr_info("unsupported transfer %d/%d, no data\n", i + 1,
				num);
			ret = -EOPNOTSUPP;
			goto end;
		}
	}

	mutex_lock(&connector->i2c_lock);
	connector->i2c_is_segment = false;
	connector->i2c_is_regaddr = false;
	connector->i2c_slave_number = 0;

	/* reset */
	if (connector->base.connector_type == DRM_MODE_CONNECTOR_HDMIA) {
		index = 0;
	} else if (connector->base.connector_type == DRM_MODE_CONNECTOR_HDMIB) {
		index = 1;
	} else if (connector->base.connector_type == DRM_MODE_CONNECTOR_DVID) {
		index = 2;
	} else {
		pr_err("Unknown connector type\n");
		ret = -EMEDIUMTYPE;
		goto end;
	}
	
	ddk770_HDMI_Check_EDID_IIC_State(index);
	if (!ddk770_HDMI_HPD_Detect(index)) {
		ret = -ENODEV;
		goto end;
	}
	/* Reducing the HDMI IIC speed to 50khz to make the IIC more stable, just like DL */
	ddk770_HDMI_Write_Register(index, 0x7e0c, 0xD8);
	ddk770_HDMI_Write_Register(index, 0x7e0e, 0xFE);

	for (i = 0; i < num; i++) {
		if (msgs[i].addr == EDID_I2C_SEGMENT_ADDR && msgs[i].len == 1) {
			connector->i2c_is_segment = true;
			connector->i2c_slave_number = (u8)*msgs[i].buf;
			continue;
		}

		if (msgs[i].flags & I2C_M_RD) {
			if (!connector->i2c_is_regaddr) {
				connector->i2c_slave_reg = 0x00;
				connector->i2c_is_regaddr = true;
			}

			if (connector->i2c_is_segment) {
				ret = ddc_read(index, EDID_I2C_ADDR,
					       EDID_I2C_SEGMENT_ADDR,
					       connector->i2c_slave_number,
					       connector->i2c_slave_reg,
					       msgs[i].len, msgs[i].buf);
				if (ret) {
					pr_err("ddc_read failed\n");
					goto end;
				}
				connector->i2c_slave_reg += msgs[i].len;
			} else {
				ret = ddc_read(index, EDID_I2C_ADDR, 0, 0,
					       connector->i2c_slave_reg,
					       msgs[i].len, msgs[i].buf);
				if (ret) {
					pr_err("ddc_read failed\n");
					goto end;
				}
				connector->i2c_slave_reg += msgs[i].len;
			}
			connector->i2c_is_regaddr = false;
		} else {
			if (!connector->i2c_is_regaddr) {
				/* Use the first write byte as register address */
				connector->i2c_slave_reg = msgs[i].buf[0];
				msgs[i].len--;
				msgs[i].buf++;
				connector->i2c_is_regaddr = true;
			}
			connector->i2c_slave_reg += msgs[i].len;
		}
	}
	ret = num;
	mutex_unlock(&connector->i2c_lock);

	return ret;

end:
	mutex_unlock(&connector->i2c_lock);
	return ret;
}

static u32 ddk770_hdmi_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm ddk770_hdmi_i2c_algo = {
	.master_xfer = ddk770_hdmi_i2c_xfer,
	.functionality = ddk770_hdmi_i2c_func
};

long ddk770_HDMI_AdaptHWI2CInit(struct smi_connector *connector)
{
	int ret;

	mutex_init(&connector->i2c_lock);

	connector->adapter.owner = THIS_MODULE;
	snprintf(connector->adapter.name, I2C_NAME_SIZE, "SMI HW I2C Bus");
	connector->adapter.dev.parent = connector->base.dev->dev;
	i2c_set_adapdata(&connector->adapter, connector);
	connector->adapter.algo = &ddk770_hdmi_i2c_algo;
	ret = i2c_add_adapter(&connector->adapter);
	if (ret) {
		pr_err("HW i2c add adapter failed. %d\n", ret);
		return -1;
	}

	return 0;
}

#endif