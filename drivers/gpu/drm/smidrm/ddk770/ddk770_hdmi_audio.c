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
#include "ddk770_hdmi_audio.h"


audio_n_computation_t n_values_48[] = {
	{0,      6144},
	{25175, 6864},
	{25200, 6144},
	{27000, 6144},
	{27027, 6144},
	{54000, 6144},
	{54054, 6144},
	{74176,11648},
	{74250, 6144},
	{148352,5824},
	{148500, 6144},
	{296703,5824},
	{297000, 5120},
	{593410, 5824},
	{594000, 6144},
	{0, 0}
};



static u32 audio_ComputeN(u32 freq, u32 pixelClk)
{
	int i = 0;
	u32 n = 0;
	audio_n_computation_t *n_values = NULL;
	int multiplier_factor = 1;

	if(u32_is_equal(freq, 64) || u32_is_equal(freq, 88) || u32_is_equal(freq, 96)){
		multiplier_factor = 2;
	}
	else if(u32_is_equal(freq, 128) || u32_is_equal(freq, 176) || u32_is_equal(freq, 192)){
		multiplier_factor = 4;
	}
	else if(u32_is_equal(freq, 256) || u32_is_equal(freq, 352) || u32_is_equal(freq, 384)){
		multiplier_factor = 8;
	}
	if(u32_is_equal(48, freq/multiplier_factor)){
		n_values = n_values_48;
	}

	for(i = 0; n_values[i].n != 0; i++){
		if(u32_is_equal(pixelClk, n_values[i].pixel_clock)){
			n = n_values[i].n * multiplier_factor;
			return n;
		}
	}

	n = n_values[0].n * multiplier_factor;
	return n;
}

static void _audio_clock_cts(hdmi_index index, u32 value)
{

	if(value > 0){
		/* 19-bit width */		
		ddk770_HDMI_write_mask(index, AUD_CTS3, AUD_CTS3_AUDCTS_MASK, (u8)(value >> 16));
		ddk770_HDMI_write_mask(index, AUD_CTS3, AUD_CTS3_CTS_MANUAL_MASK, 1);
		ddk770_HDMI_Write_Register(index, AUD_CTS2, (u8)(value >> 8));
		ddk770_HDMI_Write_Register(index, AUD_CTS1, (u8)(value >> 0));
	}
	else{
		/* Set to automatic generation of CTS values */
		ddk770_HDMI_write_mask(index, AUD_CTS3, AUD_CTS3_CTS_MANUAL_MASK, 0);
	}
}

static void _audio_clock_atomic(hdmi_index index, u8 value)
{

	ddk770_HDMI_write_mask(index, AUD_N3, AUD_N3_NCTS_ATOMIC_WRITE_MASK, value);
}
static void _audio_clock_n(hdmi_index index, u32 value)
{

	/* 19-bit width */
	ddk770_HDMI_write_mask(index, AUD_N3, AUD_N3_AUDN_MASK, (u8)(value >> 16));
	ddk770_HDMI_Write_Register(index, AUD_N2, (u8)(value >> 8));
	ddk770_HDMI_Write_Register(index, AUD_N1, (u8)(value >> 0));
	
	
	/* no shift */
	ddk770_HDMI_write_mask(index, AUD_CTS3, AUD_CTS3_N_SHIFT_MASK, 0);
}

static void mc_audio_sampler_clock_enable(hdmi_index index, u8 bit)
{

	ddk770_HDMI_write_mask(index, MC_CLKDIS, MC_CLKDIS_AUDCLK_DISABLE_MASK, bit);
}

static void fc_sample_freq(hdmi_index index, u8 sf)
{

	ddk770_HDMI_write_mask(index, FC_AUDICONF1, FC_AUDICONF1_SF_MASK, sf);
}

static void fc_coding_type(hdmi_index index, u8 codingType)
{

	ddk770_HDMI_write_mask(index, FC_AUDICONF0, FC_AUDICONF0_CT_MASK, codingType);
}

static void fc_sampling_size(hdmi_index index, u8 ss)
{

	ddk770_HDMI_write_mask(index, FC_AUDICONF1, FC_AUDICONF1_SS_MASK, ss);
}

static void fc_audio_info_config(hdmi_index index, u32 freq)
{

    u32 sampling_freq = freq;

    /* Audio InfoFrame sample frequency when OBA or DST */
    if (u32_is_equal(sampling_freq, 32))
    {
        fc_sample_freq(index, 1);
    }
    else if (u32_is_equal(sampling_freq, 42))
    {
        fc_sample_freq(index, 2);
    }
    else if (u32_is_equal(sampling_freq, 48))
    {
        fc_sample_freq(index, 3);
		printk("sample freq is 48\n");
    }
    else if (u32_is_equal(sampling_freq, 88))
    {
        fc_sample_freq(index, 4);
    }
    else if (u32_is_equal(sampling_freq, 96))
    {
        fc_sample_freq(index, 5);
    }
    else if (u32_is_equal(sampling_freq, 176))
    {
        fc_sample_freq(index, 6);
    }
    else if (u32_is_equal(sampling_freq, 192))
    {
        fc_sample_freq(index, 7);
    }
    else
    {
        fc_sample_freq(index, 0);
    }

    fc_coding_type(index, 0);   /* for HDMI refer to stream header  (0) */
    fc_sampling_size(index, 0); /* for HDMI refer to stream header  (0) */
}

static void _audio_i2s_reset_fifo(hdmi_index index)
{

	ddk770_HDMI_write_mask(index, AUD_CONF0, AUD_CONF0_SW_AUDIO_FIFO_RST_MASK, 1);
}

static void _audio_i2s_reset(hdmi_index index)
{

	ddk770_HDMI_write_mask(index, MC_SWRSTZREQ, MC_SWRSTZREQ_II2SSWRST_REQ_MASK, 0);
}
 
static void _audio_i2s_data_enable(hdmi_index index, u8 value)
{

	ddk770_HDMI_write_mask(index, AUD_CONF0, AUD_CONF0_I2S_IN_EN_MASK, value);
}

static void _audio_i2s_data_mode(hdmi_index index, u8 value)
{

	ddk770_HDMI_write_mask(index, AUD_CONF1, AUD_CONF1_I2S_MODE_MASK, value);
}

static void _audio_i2s_data_width(hdmi_index index, u8 value)
{

	ddk770_HDMI_write_mask(index, AUD_CONF1, AUD_CONF1_I2S_WIDTH_MASK, value);
}

static void audio_i2s_select(hdmi_index index, u8 bit)
{

	ddk770_HDMI_write_mask(index, AUD_CONF0, AUD_CONF0_I2S_SELECT_MASK, bit);
}

static void audio_i2s_interrupt_mask(hdmi_index index, u8 value)
{

	ddk770_HDMI_write_mask(index, AUD_INT, AUD_INT_FIFO_FULL_MASK_MASK |
								 AUD_INT_FIFO_EMPTY_MASK_MASK, value);
}

static void audio_spdif_interrupt_mask(hdmi_index index, u8 value)
{
	ddk770_HDMI_write_mask(index, AUD_SPDIFINT, AUD_SPDIFINT_SPDIF_FIFO_FULL_MASK_MASK |
									  AUD_SPDIFINT_SPDIF_FIFO_EMPTY_MASK_MASK, value);
}



static void fc_iec_clock_accuracy(hdmi_index index, u8 value)
{

	ddk770_HDMI_write_mask(index, FC_AUDSCHNL7, FC_AUDSCHNL7_OIEC_CLKACCURACY_MASK, value);
}

static void fc_iec_sampling_freq(hdmi_index index, u8 value)
{

	ddk770_HDMI_write_mask(index, FC_AUDSCHNL7, FC_AUDSCHNL7_OIEC_SAMPFREQ_MASK, value);
}

static void fc_iec_original_sampling_freq(hdmi_index index, u8 value)
{
	
	ddk770_HDMI_write_mask(index, FC_AUDSCHNL8, FC_AUDSCHNL8_OIEC_ORIGSAMPFREQ_MASK, value);
}


static void fc_iec_word_length(hdmi_index index, u8 value)
{
	ddk770_HDMI_write_mask(index, FC_AUDSCHNL8, FC_AUDSCHNL8_OIEC_WORDLENGTH_MASK, value);
}

static void fc_packet_layout(hdmi_index index, u8 bit)
{

	ddk770_HDMI_write_mask(index, FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK, bit);
}


static void fc_iec_channel_right(hdmi_index index, u8 value, unsigned channel)
{
	
	if (channel == 0)
		ddk770_HDMI_write_mask(index, FC_AUDSCHNL3, FC_AUDSCHNL3_OIEC_CHANNELNUMCR0_MASK, value);
	else if (channel == 1)
		ddk770_HDMI_write_mask(index, FC_AUDSCHNL3, FC_AUDSCHNL3_OIEC_CHANNELNUMCR1_MASK, value);
	else if (channel == 2)
		ddk770_HDMI_write_mask(index, FC_AUDSCHNL4, FC_AUDSCHNL4_OIEC_CHANNELNUMCR2_MASK, value);
	else if (channel == 3)
		ddk770_HDMI_write_mask(index, FC_AUDSCHNL4, FC_AUDSCHNL4_OIEC_CHANNELNUMCR3_MASK, value);

}

static void fc_iec_channel_left(hdmi_index index, u8 value, unsigned channel)
{
	
	if (channel == 0)
		ddk770_HDMI_write_mask(index, FC_AUDSCHNL5, FC_AUDSCHNL5_OIEC_CHANNELNUMCL0_MASK, value);
	else if (channel == 1)
		ddk770_HDMI_write_mask(index, FC_AUDSCHNL5, FC_AUDSCHNL5_OIEC_CHANNELNUMCL1_MASK, value);
	else if (channel == 2)
		ddk770_HDMI_write_mask(index, FC_AUDSCHNL6, FC_AUDSCHNL6_OIEC_CHANNELNUMCL2_MASK, value);
	else if (channel == 3)
		ddk770_HDMI_write_mask(index, FC_AUDSCHNL6, FC_AUDSCHNL6_OIEC_CHANNELNUMCL3_MASK, value);

}



static void _audio_clock_f(hdmi_index index, u8 value)
{
	
	ddk770_HDMI_write_mask(index, AUD_INPUTCLKFS, AUD_INPUTCLKFS_IFSFACTOR_MASK, value);
}


static void fc_audio_config(hdmi_index index)
{
	int i = 0;
	
	fc_packet_layout(index, 0); /// More than 2 channels => layout 1 else layout 0

	for (i = 0; i < 4; i++) {	/* 0, 1, 2, 3 */
		fc_iec_channel_left(index, 2 * i + 1, i);	/* 1, 3, 5, 7 */
		fc_iec_channel_right(index, 2 * (i + 1), i);	/* 2, 4, 6, 8 */
	}

	fc_iec_clock_accuracy(index, 0);

	fc_iec_sampling_freq(index, 0x2);

	fc_iec_original_sampling_freq(index, 0xD);

	fc_iec_word_length(index, 0xB);    //word width
}





int audio_Initialize(hdmi_index index)
{


	// Mask all interrupts
	audio_i2s_interrupt_mask(index, 0x3);
	audio_spdif_interrupt_mask(index, 0x3);

	return 0;
}




void audio_Configure(hdmi_index index, u32 pixel_clock, u32 samplefreq)
{
    u32 n = 0;

	fc_audio_mute(index);

	// Configure Frame Composer audio parameters
	fc_audio_config(index);

	audio_i2s_configure(index, SAMPLE_SIZE );	
	//if set HDMI_AUD_INPUTCLKFS_128FS,TV does not have sound.
	_audio_clock_f(index, 4); //HDMI_AUD_INPUTCLKFS_64FS;

    n = audio_ComputeN(samplefreq, pixel_clock);
	
	printk("Audio N=%d\n",n);
	
	mc_audio_sampler_clock_enable(index, 0);
	
	usleep_range(1000, 1100);

	_audio_clock_atomic(index, 1);
	
	_audio_clock_cts(index, 0);
	
	_audio_clock_n(index, n);		

	fc_audio_unmute(index);

	// Configure audio info frame packets
	fc_audio_info_config(index, samplefreq);

	
}

void audio_i2s_configure(hdmi_index index, int sampleSize)
{
	audio_i2s_select(index, 1);

	_audio_i2s_data_enable(index, 0x1); // I2S_in_en[0] - I2Sdata[0] enable

	/* ATTENTION: fixed I2S data mode (standard) */
	_audio_i2s_data_mode(index, 0x0);
	_audio_i2s_data_width(index, sampleSize);
	audio_i2s_interrupt_mask(index, 3);
	_audio_i2s_reset_fifo(index);
	_audio_i2s_reset(index);
}



void fc_audio_mute(hdmi_index index)
{
	ddk770_HDMI_write_mask(index, FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK, 0xF);
	_audio_i2s_data_enable(index, 0x0); 
}

void fc_audio_unmute(hdmi_index index)
{
	_audio_i2s_data_enable(index, 0x1); 
	ddk770_HDMI_write_mask(index, FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK, 0x0);
	
}



