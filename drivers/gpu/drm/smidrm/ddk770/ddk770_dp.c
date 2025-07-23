#ifdef __linux__
#include <linux/unistd.h>
#include <linux/string.h>
#endif
#include "ddk770_reg.h"
#include "ddk770_dp.h"
#include "ddk770_help.h"
#include "ddk770_hwi2c.h"
#include "ddk770_display.h"
#include "ddk770_chip.h"
#include "ddk770_mode.h"
#include "ddk770_hdmi_ddc.h"
#include <linux/delay.h>
#include <linux/timer.h>


#define BIT1(x) (0x1 << x)
#define min1(a, b) ((a) < (b) ? (a) : (b))
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

dp_info g_dp_info[2] = { {0}, {0} };

unsigned int pll_table[][4] = {
	    /* prediv, fbdiv, postdiv, clkdiv_16m */
		{  0x02,   0x87,  0x01,    0x0d},
		{  0x02,   0xe1,  0x01,    0x15},
		{  0x02,   0xe1,  0x00,    0x2a},
};

static void DP_Write32(dp_index index, unsigned int offset, unsigned int value)
{
    pokeRegisterDWord(DP_BASE + (DP_OFFSET * index) + offset, value);
	// printk("write 0x%x 0x%x\n", DP_BASE + (DP_OFFSET * index) + offset, value);
}

static unsigned int DP_Read32(dp_index index, unsigned int offset)
{
    return peekRegisterDWord(DP_BASE + (DP_OFFSET * index) + offset);
}

static void DP_AUX_Request(dp_index index)
{
    DP_Write32(index, DP_CONTROLLER418, 
                    FIELD_SET(DP_Read32(index, DP_CONTROLLER418), DP_CONTROLLER418, CFG_PHY_AUX_START, TRUE));
}

static unsigned int DP_Aux_Write(dp_index index, unsigned int cmd, unsigned int addr, unsigned int * wr_buf, unsigned int length)
{
    unsigned int val;
	int i;

    //Read Request Transaction
    val = DP_Read32(index, DP_CONTROLLER400);
    val = FIELD_VALUE(val, DP_CONTROLLER400, CFG_PHY_AUX_DATA_CMD, cmd);
    val = FIELD_VALUE(val, DP_CONTROLLER400, CFG_PHY_AUX_DATA_ADDR, addr);
    val = FIELD_VALUE(val, DP_CONTROLLER400, CFG_PHY_AUX_DATA_LENGTH, length);
    DP_Write32(index, DP_CONTROLLER400, val);

    //Write Request Transaction
    if (!(cmd & 0x1))
    {
        for (i = 0; i < 4; i++)
            DP_Write32(index, DP_CONTROLLER408 + i * 4, wr_buf[i]);
    }

    DP_AUX_Request(index);

    return DP_SUCCESS;
}

static unsigned int DP_Aux_Read(dp_index index, unsigned int read, unsigned int addr, unsigned int * rd_buf)
{
    unsigned int ret, retry = 0;
	int i;
    //unsigned DPBaseAddr = DP_BASE + (DP_OFFSET * index);

    while (retry++ <= 30) {
        if (FIELD_VAL_GET(DP_Read32(index, DP_TOP80), DP_TOP80, STA_AUX_REPLY_EVENT))
            break;
		usleep_range(1000, 1100);
	}

    DP_Write32(index, DP_TOP80, 
                FIELD_SET(DP_Read32(index, DP_TOP80), DP_TOP80, STA_AUX_REPLY_EVENT, TRUE));

    if (read == 1)
    {
		usleep_range(2000, 2100);
        for (i = 0; i < 4; i++)
            rd_buf[i] = DP_Read32(index, DP_CONTROLLER408 + i * 4);
    }
    
    ret = FIELD_VAL_GET(DP_Read32(index, DP_CONTROLLER404), DP_CONTROLLER404, AUX_REPLY_CMD);

    return ret;
}

static unsigned int DP_Aux_Channel_Run(dp_index index, struct aux_cfg *aux_get_rx)
{
	unsigned int ret = 0;

	ret = DP_Aux_Write(index, aux_get_rx->aux_cmd,
			aux_get_rx->dpcd_addr, aux_get_rx->wr_buff,
			aux_get_rx->length);
	if (ret == 1)
		return 1;

	ret = DP_Aux_Read(index, aux_get_rx->read,
			aux_get_rx->dpcd_addr, aux_get_rx->rd_buff);

	return ret;
}

static int DP_Aux_Msg(dp_index index, struct aux_cfg *aux_msg)
{
	int ret = 0;
	unsigned int retry = 0, max_retry = 7;

	for (retry = 0; retry < max_retry; retry++) {
		ret = DP_Aux_Channel_Run(index, aux_msg);
		if (ret == DP_AUX_REPLY_DEFER || ret == DP_AUX_I2C_REPLY_DEFER) {		
			usleep_range(2000, 2100);
			continue;
		} else {
			break;
		}
	}

	return ret;
}

static int DP_Aux_Transfer(dp_index index, unsigned char request,
	unsigned int offset, void *buffer, int size)
{
	int ret = 0, j = 0;
	unsigned int wr_buf[4] = {0};
	unsigned int rd_buf[4] = {0};
	struct aux_cfg aux_link = { 0 };

	if (size > DP_AUX_MAX_PAYLOAD_BYTES)
		return -1;

	aux_link.wr_buff = wr_buf;
	aux_link.rd_buff = rd_buf;
	aux_link.aux_cmd = request;
	aux_link.dpcd_addr = offset;
	aux_link.length = size ? size - 1 : 0x10;

	switch (request & ~DP_AUX_I2C_MOT) {
	case DP_AUX_NATIVE_WRITE:
	case DP_AUX_I2C_WRITE:
	case DP_AUX_I2C_WRITE_STATUS_UPDATE:
	{
		aux_link.read = 0;
		if (size && buffer) {
			for (j = 0; j < size; j++)
				aux_link.wr_buff[j / 4] |= (((unsigned char *)buffer)[j] << ((j % 4) * 8));
		}
		ret = DP_Aux_Msg(index, &aux_link);
	}
	break;
	case DP_AUX_NATIVE_READ:
	case DP_AUX_I2C_READ:
	{
		aux_link.read = 1;
		ret = DP_Aux_Msg(index, &aux_link);
		if (size && buffer) {
			for (j = 0; j < size; j++)
				((unsigned char *)buffer)[j] = (aux_link.rd_buff[j / 4] >> (8 *(j % 4))) & 0xff;
		}
	}
	break;
	default:
		ret = -1;
	break;
	}

	if (ret)
		return -1;
	else
		return size;
}

static int DP_DPCD_Access(dp_index index, unsigned char request,
	unsigned int offset, void *buffer, int size)
{
	void *msg_buf = NULL;
	int transfer_size = DP_AUX_MAX_PAYLOAD_BYTES;
	int i = 0, msg_size = 0, msg_offset = 0, ret = 0;

	if (!buffer)
		return -1;

	for (i = 0; i < size; i += msg_size) {
		msg_buf = (unsigned char*)buffer + i;
		msg_size = min1(size - i, transfer_size);
		msg_offset = offset + i;
		ret += DP_Aux_Transfer(index, request, msg_offset, msg_buf, msg_size);
	}

	return ret;
}

static int DP_DPCD_Write(dp_index index, unsigned int offset, void *buffer, int size)
{
	return DP_DPCD_Access(index, DP_AUX_NATIVE_WRITE, offset, buffer, size);
}

static int DP_DPCD_Read(dp_index index, unsigned int offset, void *buffer, int size)
{
	return DP_DPCD_Access(index, DP_AUX_NATIVE_READ, offset, buffer, size);
}

void DP_Audio_Reset(dp_index index)
{

	DP_Write32(index, DP_TOP1C,
			FIELD_SET(DP_Read32(index, DP_TOP1C), DP_TOP1C, REG_RESET_AUDIO, TRUE));
	DP_Write32(index, DP_TOP1C,
			FIELD_SET(DP_Read32(index, DP_TOP1C), DP_TOP1C, REG_RESET_AUDIO, FALSE));
}



void DP_Audio_CheckOverrun(dp_index index)
{
	int timeout = 100;

	
	// enable Audio overun event
	DP_Write32(index, DP_TOP84, 
				FIELD_SET(DP_Read32(index, DP_TOP84), DP_TOP84, ENABLE_AUD_OVERRUN, TRUE));   
	

   	while ((FIELD_VAL_GET(DP_Read32(index,DP_TOP80), DP_TOP80, STA_AUD_OVERRUN) == DP_TOP80_STA_AUD_OVERRUN_TRUE) && timeout--) 
	{


		DP_Write32(index,DP_TOP80,FIELD_SET(DP_Read32(index, DP_TOP80), DP_TOP80, STA_AUD_OVERRUN, TRUE)); //Clear status

		DP_Audio_Reset(index);

	}

}

/*
 * Note:You must read the monitor's edid to determine if the monitor
 * supports audio before enabling the audio function.
 */
static void DP_Audio_Start(dp_index index, int bits, int channel)
{
	 unsigned int regval;

    /* sdp_audio_stream_vertical enable */
	/* sdp_audio_timestamp_vertical enable */
	DP_Write32(index, DP_CONTROLLER500,
				FIELD_SET(0, DP_CONTROLLER500, SDP_AUDIO_STREAM_VERTICAL, ENABLE) |
				FIELD_SET(0, DP_CONTROLLER500, SDP_AUDIO_TIMESTAMP_VERTICAL, ENABLE));

	/* sdp_audio_stream_horizontal_enable */
	/* sdp_audio_timestamp_horizontal enable */
	DP_Write32(index, DP_CONTROLLER504,
				FIELD_SET(0, DP_CONTROLLER504, SDP_AUDIO_STREAM_HORIZONTAL, ENABLE) |
				FIELD_SET(0, DP_CONTROLLER504, SDP_AUDIO_TIMESTAMP_HORIZONTAL, ENABLE));


	regval = DP_Read32(index, DP_CONTROLLER300);
	
    if (channel == 2)
		regval = FIELD_VALUE(regval, DP_CONTROLLER300, AUDIO_DATA_EN,  DP_CONTROLLER300_AUDIO_DATA_EN_12);
	else // sampleChannels == 8
		regval = FIELD_VALUE(regval, DP_CONTROLLER300, AUDIO_DATA_EN,  DP_CONTROLLER300_AUDIO_DATA_EN_ALL);


	regval = FIELD_VALUE(regval, DP_CONTROLLER300, AUDIO_DATA_WIDTH,  bits);
	regval = FIELD_VALUE(regval, DP_CONTROLLER300, AUDIO_CHANNEL_NUM, channel>2?channel:(channel-1));
	regval = FIELD_SET(regval, DP_CONTROLLER300, AUDIO_LEGEND, ALL); /* indicates channels 1,2 input data is vaild */

	DP_Write32(index, DP_CONTROLLER300,regval);

	//disable AUD MUTE
    DP_Write32(index, DP_CONTROLLER300,
                FIELD_SET(DP_Read32(index, DP_CONTROLLER300), DP_CONTROLLER300, AUDIO_MUTE, CLEAR));

	DP_Write32(index, 0x304, 0);

    //reset audio,
			DP_Audio_Reset(index);

}

__attribute__((unused)) static int DP_Audio_Stop(dp_index index)
{
	DP_Write32(index, DP_CONTROLLER300, 
				FIELD_VALUE(DP_Read32(index, DP_CONTROLLER300), DP_CONTROLLER300, AUDIO_DATA_EN, DP_CONTROLLER300_AUDIO_DATA_EN_0));  /* indicates channels input data invaild */

	/* sdp_audio_stream_vertical disable */
	/* sdp_audio_timestamp_vertical disable */
	DP_Write32(index, DP_CONTROLLER500,
				FIELD_SET(0, DP_CONTROLLER500, SDP_AUDIO_TIMESTAMP_VERTICAL, DISABLE));

	/* sdp_audio_stream_horizontal_disable */
	/* sdp_audio_timestamp_horizontal disable */
	DP_Write32(index, DP_CONTROLLER504,
				FIELD_SET(0, DP_CONTROLLER504, SDP_AUDIO_TIMESTAMP_HORIZONTAL, DISABLE));

	return 0;
}




void DP_Audio_Mute(dp_index index)
{
	
	unsigned int regval;

	regval = DP_Read32(index, DP_CONTROLLER300);
	regval = FIELD_SET(regval, DP_CONTROLLER300, AUDIO_MUTE, SET);
	regval = FIELD_VALUE(regval, DP_CONTROLLER300, AUDIO_DATA_EN, 0);
	regval = FIELD_VALUE(regval, DP_CONTROLLER300, AUDIO_CHANNEL_NUM, DP_CONTROLLER300_AUDIO_CHANNEL_NUM_1);

	DP_Write32(index, DP_CONTROLLER300,regval);
	

}


void DP_Audio_UnMute(dp_index index)
{

	unsigned int regval;

	regval = DP_Read32(index, DP_CONTROLLER300);

	regval = FIELD_SET(regval, DP_CONTROLLER300, AUDIO_MUTE, CLEAR);
	regval = FIELD_VALUE(regval, DP_CONTROLLER300, AUDIO_DATA_EN,  DP_CONTROLLER300_AUDIO_DATA_EN_12);
	regval = FIELD_VALUE(regval, DP_CONTROLLER300, AUDIO_CHANNEL_NUM, DP_CONTROLLER300_AUDIO_CHANNEL_NUM_2);

	DP_Write32(index, DP_CONTROLLER300,regval);


}



static void DP_Drv_Init(dp_index index)
{
    // phy soft reset 
    DP_Write32(index, DP_TOP1C,  // set reg0x001c[1] to 1
                FIELD_SET(DP_Read32(index, DP_TOP1C), DP_TOP1C, REG_RESET_PHY, TRUE));

    usleep_range(1000, 1100);

    DP_Write32(index, DP_TOP1C,  // set reg0x001c[1] to 0
                FIELD_SET(DP_Read32(index, DP_TOP1C), DP_TOP1C, REG_RESET_PHY, FALSE));

	//dpureg(0x100), fast sim, disable lane channel
    DP_Write32(index, DP_PHY100, (0x1 << 31) |
                FIELD_SET(DP_Read32(index, DP_PHY100), DP_PHY100, TRANSMIT_ENABLE, NONE));
}

static void DP_Aux_Init(dp_index index)
{
    DP_Write32(index, 0x1a0, BIT1(15) | BIT1(14) |
                DP_Read32(index, 0x1a0));

  	DP_Write32(index, DP_ANALOG180, 
                FIELD_SET(0, DP_ANALOG180, SSCG, DISABLE));
    DP_Write32(index, DP_ANALOG188,
                FIELD_VALUE(DP_Read32(index, DP_ANALOG188), DP_ANALOG188, DA_COREPLL_POSTDIV, 0x1));

    DP_Write32(index, 0x18c, 0x2a);

    DP_Write32(index, DP_ANALOG180, 
              	FIELD_SET(0, DP_ANALOG180, SSCG, DISABLE) |
				FIELD_VALUE(0, DP_ANALOG180, DA_COREPLL_PREDIV, 0x01) |
				FIELD_VALUE(0, DP_ANALOG180, COREPLL_FBDIV, 0xe1));

	usleep_range(1000, 1100);
}

static void DP_Training_Pattern_Set(dp_index index, unsigned int pattern)
{
	unsigned int temp = 0;

	/* dpureg(0x100 DP_LANE_CONFIG) */
    temp = DP_Read32(index, DP_PHY100);
	temp &= ~0xf;
	temp |= BIT1(16);
	temp |= pattern;
	DP_Write32(index, DP_PHY100, temp);
}

static void DP_Phy_Cfg(dp_index index)
{
	unsigned char lane_en = (1 << g_dp_info[index].lane_count) - 1;

	DP_Write32(index, DP_PHY100, 
				FIELD_VALUE(0, DP_PHY100, TRANSMIT_ENABLE, lane_en) |
				FIELD_VALUE(0, DP_PHY100, PHY_LANES, g_dp_info[index].phy_lanes) |
				FIELD_VALUE(0, DP_PHY100, PHY_RATE, g_dp_info[index].phy_rate));

	if (g_dp_info[index].enhance_mode) {
        DP_Write32(index, DP_TOP18, 
                    FIELD_SET(DP_Read32(index, DP_TOP18), DP_TOP18, ENHANCE_FRAMING, ENABLE));
	} else {
        DP_Write32(index, DP_TOP18, 
                    FIELD_SET(DP_Read32(index, DP_TOP18), DP_TOP18, ENHANCE_FRAMING, DISABLE));
	}
}

static void DP_Core_PLL_Cfg(dp_index index)
{


	if (g_dp_info[index].phy_rate >= ARRAY_SIZE(pll_table) || g_dp_info[index].phy_rate < 0)
		g_dp_info[index].phy_rate = 1;

	DP_Write32(index, DP_ANALOG188,  //dpureg(0x188), DP_SPREAD_CFG
				FIELD_VALUE(DP_Read32(index, DP_ANALOG188), DP_ANALOG188, DA_COREPLL_POSTDIV,
							(unsigned char) (pll_table[g_dp_info[index].phy_rate][pll_postdiv])));

	DP_Write32(index, 0x18c, //dpureg(0x18c), DP_CLKDIV_16M
				pll_table[g_dp_info[index].phy_rate][pll_clkdiv_16m]);

	DP_Write32(index, DP_ANALOG180,	//dpureg(0x180)
				FIELD_VALUE(0, DP_ANALOG180, DA_COREPLL_FBDIV,
							(unsigned char) (0xf0 | ((pll_table[g_dp_info[index].phy_rate][pll_fbdiv] >> 8) & 0xf)) & 0xf) |
				FIELD_VALUE(0, DP_ANALOG180, COREPLL_FBDIV,
							(unsigned char) (pll_table[g_dp_info[index].phy_rate][pll_fbdiv] & 0xff)) |
				FIELD_VALUE(0, DP_ANALOG180, DA_COREPLL_PREDIV,
							(unsigned char) (0x0 | (pll_table[g_dp_info[index].phy_rate][pll_prediv] & 0x1f))) |
				FIELD_SET(0, DP_ANALOG180, SSCG, DISABLE));

	usleep_range(1000, 1100);

}

#define DIV_ROUNDUP(n, d) (((n) + (d) - 1) / (d))


static void DP_Tu_Init(dp_index index)
{
	unsigned short htotal = g_dp_info[index].cea_mode->h_active + g_dp_info[index].cea_mode->h_blank;
	//unsigned short hactive = g_dp_info[index].cea_mode->h_active;
	unsigned short hblank = g_dp_info[index].cea_mode->h_blank;
	unsigned int clock = g_dp_info[index].cea_mode->tmds_clk / 1000;
	unsigned int hb_num = 0;
	unsigned int tu;
	unsigned int tu_frac;
	unsigned int tu_int;
	unsigned int rd_thres;
	unsigned int link_rate = 0;

	if (!htotal)
		return;

	if (g_dp_info[index].phy_rate == 0x00) {	     // 162GBPS
		hb_num = hblank * 405 / clock/10; //(162 / 4)
		link_rate = 162;
	} else if (g_dp_info[index].phy_rate == 0x1) {	 // 270GBPS
		hb_num = hblank * 675 / clock/10; //(270 / 4) 
		link_rate = 270;
	} else if (g_dp_info[index].phy_rate == 0x2) {   // 540GBPS
		hb_num = hblank * 135 / clock;  //(540 / 4) 
		link_rate = 540;
	}

	tu = DIV_ROUNDUP(clock * 24 * 640 , 8 * g_dp_info[index].lane_count * link_rate) ;
	 
	tu_frac = tu % 10;
	tu_int = tu / 10;

	if (tu_int < 6) {
		rd_thres = 32;
	} else if (hblank < 80) {
		rd_thres = 12;
	} else {
		rd_thres = 16;
	}

	// hblank *link_clk/pixel_clk; link_clk = symbol clock /4; @1.62G --> 280*(162/4)/148.5 = 76.3636
	DP_Write32(index, DP_CONTROLLER230,
				FIELD_VALUE(0, DP_CONTROLLER230, HBLANK_LINK_CYC, hb_num));
	// tu = 148.5*24*640 / 8*4*1620 = 44
	DP_Write32(index, DP_CONTROLLER220,
				FIELD_VALUE(0, DP_CONTROLLER220, REG_LINE_RD_THRES, rd_thres) |
				FIELD_VALUE(0, DP_CONTROLLER220, REG_AVG_PER_TU_INT, tu_int) |
				FIELD_VALUE(0, DP_CONTROLLER220, REG_AVG_PER_TU_FRAC, tu_frac));
}

static unsigned char DP_Link_Status(const unsigned char link_status[DP_LINK_STATUS_SIZE], int r)
{
	return link_status[r - DP_LANE0_1_STATUS];
}

static unsigned char DP_Get_Lane_Status(const unsigned char link_status[DP_LINK_STATUS_SIZE],
			     int lane)
{
	int i = DP_LANE0_1_STATUS + (lane >> 1);
	int s = (lane & 1) * 4;
	unsigned char l = DP_Link_Status(link_status, i);

	return (l >> s) & 0xf;
}

static unsigned char DP_Get_Adjust_Request_Voltage(const unsigned char link_status[DP_LINK_STATUS_SIZE],
				     int lane)
{
	int i = DP_ADJUST_REQUEST_LANE0_1 + (lane >> 1);
	int s = ((lane & 1) ?
		 DP_ADJUST_VOLTAGE_SWING_LANE1_SHIFT :
		 DP_ADJUST_VOLTAGE_SWING_LANE0_SHIFT);
	unsigned char l = DP_Link_Status(link_status, i);

	return ((l >> s) & 0x3) << DP_TRAIN_VOLTAGE_SWING_SHIFT;
}

static unsigned char DP_Get_Adjust_Request_Pre_Emphasis(const unsigned char link_status[DP_LINK_STATUS_SIZE],
					  int lane)
{
	int i = DP_ADJUST_REQUEST_LANE0_1 + (lane >> 1);
	int s = ((lane & 1) ?
		 DP_ADJUST_PRE_EMPHASIS_LANE1_SHIFT :
		 DP_ADJUST_PRE_EMPHASIS_LANE0_SHIFT);
	unsigned char l = DP_Link_Status(link_status, i);

	return ((l >> s) & 0x3) << DP_TRAIN_PRE_EMPHASIS_SHIFT;
}

static bool DP_Clk_Recovery_OK(const unsigned char link_status[DP_LINK_STATUS_SIZE],
			      int lane_count)
{
	int lane;
	unsigned char lane_status;

	for (lane = 0; lane < lane_count; lane++) {
		lane_status = DP_Get_Lane_Status(link_status, lane);
		if ((lane_status & DP_LANE_CR_DONE) == 0)
			return false;
	}
	return true;
}

static bool DP_Channel_EQ_OK(const unsigned char link_status[DP_LINK_STATUS_SIZE],
			  int lane_count)
{
	unsigned char lane_align;
	unsigned char lane_status;
	int lane;

	lane_align = DP_Link_Status(link_status,
				    DP_LANE_ALIGN_STATUS_UPDATED);
	if ((lane_align & DP_INTERLANE_ALIGN_DONE) == 0)
		return false;
	for (lane = 0; lane < lane_count; lane++) {
		lane_status = DP_Get_Lane_Status(link_status, lane);
		if ((lane_status & DP_CHANNEL_EQ_BITS) != DP_CHANNEL_EQ_BITS)
			return false;
	}
	return true;
}

static inline bool
DP_Enhanced_Frame_Cap(const unsigned char dpcd[DP_RECEIVER_CAP_SIZE])
{
	return dpcd[DP_DPCD_REV] >= 0x11 &&
		(dpcd[DP_MAX_LANE_COUNT] & DP_ENHANCED_FRAME_CAP);
}

static void DP_Get_Adjust_Train(dp_index index,
		const unsigned char link_status[DP_LINK_STATUS_SIZE])
{
	unsigned char post_cursor;
	int lane;
	DP_DPCD_Read(index, DP_ADJUST_REQUEST_POST_CURSOR2, &post_cursor, sizeof(post_cursor));

	for (lane = 0; lane < g_dp_info[index].lane_count; lane++) {
		g_dp_info[index].swing[lane] = DP_Get_Adjust_Request_Voltage(link_status, lane);
		g_dp_info[index].preem[lane] = DP_Get_Adjust_Request_Pre_Emphasis(link_status, lane);
		g_dp_info[index].pcursor[lane] = (post_cursor >> (lane << 1)) & 0x3;
	}
}

static void DP_Voltage_Swing_Adjust(dp_index index)
{
	unsigned char train_set[4];
	int i, lane;

	/* FROM KERNEL CODE!!! write currently selected post-cursor level (if supported) */
	if (g_dp_info[index].dpcd_rev >= 0x12 && g_dp_info[index].lane_rate == DP_LINK_BW_5_4)
	{
		train_set[0] = train_set[1] = 0;

		for (i = 0; i < g_dp_info[index].lane_count; i++)
			train_set[i / 2] |= (((g_dp_info[index].pcursor[i]) & 0x3) << (((i) & 1) << 2));

		DP_DPCD_Write(index, DP_TRAINING_LANE0_1_SET2, train_set,
								(((g_dp_info[index].lane_count) + (2) - 1) / (2)));
	}

	for (lane = 0; lane < g_dp_info[index].lane_count; lane++) {
		train_set[lane] = g_dp_info[index].swing[lane];
		if (g_dp_info[index].swing[lane] >= DP_TRAIN_VOLTAGE_SWING_LEVEL_2)
			train_set[lane] |= DP_TRAIN_MAX_SWING_REACHED;

		train_set[lane] |= (g_dp_info[index].preem[lane]);
		if (g_dp_info[index].preem[lane] >= DP_TRAIN_PRE_EMPH_LEVEL_2)
			train_set[lane] |= DP_TRAIN_MAX_PRE_EMPHASIS_REACHED;
	}

	DP_DPCD_Write(index, DP_TRAINING_LANE0_SET,
		train_set, g_dp_info[index].lane_count);
}

static int dp_bw_table[] = {
	DP_LINK_BW_1_62,
	DP_LINK_BW_2_7,
	DP_LINK_BW_5_4,
	DP_LINK_BW_8_1,
};

static int DP_Rate_Index(const int *rates, int len, int rate)
{
	int i;

	for (i = 0; i < len; i++)
		if (rate == rates[i])
			return i;

	return -1;
}

static void DP_EDP_Init_DPCD(dp_index index)
{
	unsigned char edp_rev = 0;
	int val = 0, i = 0;
	unsigned char sink_rates[DP_MAX_SUPPORTED_RATES * 2];

	/* Read the eDP display control registers. */
	DP_DPCD_Read(index, DP_EDP_DPCD_REV, &edp_rev, 1);

	if (edp_rev >= DP_EDP_14) {
		/* Read the eDP 1.4+ supported link rates. */
		DP_DPCD_Read(index, DP_SUPPORTED_LINK_RATES, sink_rates, sizeof(sink_rates));

		for (i = 0; i < DP_MAX_SUPPORTED_RATES; i++) {
			val = sink_rates[i * 2 + 1] << 8 |
				sink_rates[i * 2 + 0];

			if (val == 0)
				break;

			/* Value read multiplied by 200kHz gives the per-lane
			 * link rate in kHz.
			 */
			g_dp_info[index].sink_rates[i] = (val * 200) / 270000;

			if (g_dp_info[index].sink_rates[i] > g_dp_info[index].max_sink_rates &&
				g_dp_info[index].sink_rates[i] <= DP_LINK_BW_5_4 &&
				DP_Rate_Index(dp_bw_table, ARRAY_SIZE(dp_bw_table), g_dp_info[index].sink_rates[i]) >= 0) {
				g_dp_info[index].max_sink_rates = g_dp_info[index].sink_rates[i];
			}
		}
		g_dp_info[index].num_sink_rates = i;
	}
}

static void DP_EDP_Rate_Set(dp_index index)
{
	unsigned char rate_select = 0;

	if (g_dp_info[index].max_sink_rates) {
		rate_select = DP_Rate_Index(g_dp_info[index].sink_rates,
				g_dp_info[index].num_sink_rates, g_dp_info[index].lane_rate);

		if (rate_select >= 0) {
			DP_DPCD_Write(index, DP_LINK_RATE_SET, &rate_select, 1);
		}
	}
}

static void DP_CDR_Training(dp_index index)
{
    int i = 0;
    //unsigned int  value;
    unsigned char link_cfg[2];
    unsigned char link_status[DP_LINK_STATUS_SIZE];

    DP_Training_Pattern_Set(index, DP_TRAINING_PATTERN_1);

    DP_EDP_Rate_Set(index);

    /* Write the link configuration data */
    link_cfg[0] = g_dp_info[index].lane_rate;
    link_cfg[1] = g_dp_info[index].lane_count | (g_dp_info[index].enhance_mode << 7); /* DP_LANE_COUNT_SET */
    DP_DPCD_Write(index, DP_LINK_BW_SET, link_cfg, 2);

    link_cfg[0] = 0;
	link_cfg[1] = DP_DP_PHY108_SET_ANSI_8B10B;
	DP_DPCD_Write(index, DP_PHY108, link_cfg, 2);

    link_cfg[0] = DP_TRAINING_PATTERN_1; /* DP_TRAINING_PATTERN_SET */
	DP_DPCD_Write(index, DP_TRAINING_PATTERN_SET, link_cfg, 1);

    if (g_dp_info[index].cdr_delay <= 0)
		g_dp_info[index].cdr_delay = 4;
	else
		g_dp_info[index].cdr_delay *= 4;

    for (i = 0; i < 15; i++) {

		usleep_range(g_dp_info[index].cdr_delay * 1000, g_dp_info[index].cdr_delay * 1100);

		if (DP_DPCD_Read(index, DP_LANE0_1_STATUS, link_status,
			DP_LINK_STATUS_SIZE) != DP_LINK_STATUS_SIZE) {
			return;
		}

		if (DP_Clk_Recovery_OK(link_status, g_dp_info[index].lane_count)) {
			printk("dp[%d] get DPCD 0x%.2x%.2x, tps1 pass\n", index, link_status[0], link_status[1]);
			break;
		} else {
			printk("dp[%d] get DPCD 0x%.2x%.2x, tps1 fail, retry\n", index, link_status[0], link_status[1]);
			DP_Get_Adjust_Train(index, link_status);
			DP_Voltage_Swing_Adjust(index);
		}
	}
}

static void DP_EQ_Training(dp_index index)
{
	int i = 0, delay = 0;
	unsigned char link_cfg[2];
	unsigned char link_status[DP_LINK_STATUS_SIZE];

	DP_Training_Pattern_Set(index, DP_TRAINING_PATTERN_2);

	link_cfg[0] = DP_TRAINING_PATTERN_2; /* DP_TRAINING_PATTERN_SET */
	DP_DPCD_Write(index, DP_TRAINING_PATTERN_SET, link_cfg, 1);

	if (g_dp_info[index].eq_delay <= 0)
		g_dp_info[index].eq_delay = 4;
	else
		g_dp_info[index].eq_delay *= 4;

	delay = g_dp_info[index].eq_delay;

	for (i = 0; i < 15; i++) {

		usleep_range(delay*1000, delay*1100);

        if (DP_DPCD_Read(index, DP_LANE0_1_STATUS, link_status,
			DP_LINK_STATUS_SIZE) != DP_LINK_STATUS_SIZE) {
			return;
		}

        if (DP_Channel_EQ_OK(link_status, g_dp_info[index].lane_count)) {
			printk("dp[%d] get DPCD 0x%.2x%.2x, tps2 pass\n", index, link_status[0], link_status[1]);
			break;
		} else {
			printk("dp[%d] get DPCD 0x%.2x%.2x, tps2 fail, retry\n", index, link_status[0], link_status[1]);
			DP_Get_Adjust_Train(index, link_status);
			DP_Voltage_Swing_Adjust(index);
		}

		// Sometimes, we won't be able to get the correct status because of the lack of delay.
		delay = g_dp_info[index].eq_delay * 10;
	}
}

static void DP_Link_Start(dp_index index)
{
	unsigned char link_cfg[9] = {0};

	DP_Training_Pattern_Set(index, DP_TRAINING_PATTERN_DISABLE);

	link_cfg[0] = DP_TRAINING_PATTERN_DISABLE;
	DP_DPCD_Write(index, DP_TRAINING_PATTERN_SET, link_cfg, 1);
}

static int DP_Sink_Lane_Status_Get(dp_index index, unsigned char status[DP_LINK_STATUS_SIZE])
{
	if (DP_DPCD_Read(index, DP_LANE0_1_STATUS, status, DP_LINK_STATUS_SIZE) == DP_LINK_STATUS_SIZE)
		return 0;

	return -1;
}

static void DP_Link_Cfg(dp_index index)
{
	DP_Core_PLL_Cfg(index);
	// ebable SSC


//	DP_SSC_Enable(index,1,0,0);
	

	DP_Phy_Cfg(index);
	DP_Tu_Init(index);
}


static bool DP_Rate_Valid(int lane_rate, int lane_count, int clock, int bpc)
{
	unsigned long requirement, capacity;

	/* bandwidth: rate * lane_count
	 * rate = lane_rate * 27 unit(GBps)
	 * For more detailed information see the DP specification
	 */

	capacity = lane_rate * 27 * 1000 * 8 * lane_count;
	requirement = clock * bpc * 3 ;

	if (capacity >= requirement)
		return true;

	return false;
}

static int DP_Rate_Adjust(dp_index index)
{
	unsigned char reduce_lane_rate = 0;
	unsigned char reduce_phy_rate = 0;
	int i = 0;

	i = DP_Rate_Index(dp_bw_table, ARRAY_SIZE(dp_bw_table), g_dp_info[index].lane_rate);
	if (i > 0 && i < ARRAY_SIZE(dp_bw_table)) {
		reduce_lane_rate = dp_bw_table[i - 1];
		reduce_phy_rate = i - 1;
	} else {
		return -1;
	}

	if (DP_Rate_Valid(reduce_lane_rate, g_dp_info[index].lane_count,
		g_dp_info[index].cea_mode->tmds_clk, 8) && i >= 0) {
		g_dp_info[index].lane_rate = reduce_lane_rate;
		g_dp_info[index].phy_rate = reduce_phy_rate;
		return 0;
	}

	return -1;
}

static unsigned char DP_Check_Sink_Power(dp_index index)
{
	return g_dp_info[index].sink_power_status;
}
static int DP_Link_Train_Lock(dp_index index)
{
	unsigned char link_status[DP_LINK_STATUS_SIZE];

	if (!DP_Sink_Lane_Status_Get(index, link_status) &&
        (!DP_Clk_Recovery_OK(link_status, g_dp_info[index].lane_count) || !DP_Channel_EQ_OK(link_status, g_dp_info[index].lane_count)))
		return false;

	return true;
}

int DP_Check_Sink_Status(dp_index index)
{
	unsigned char ret = 1;

	// The powerStatus of some displays does not turn on when it is 0.
	ret = DP_Check_Sink_Power(index);
	if (ret == 2)
		return -1;
	// Sometimes, the training results may change, requiring re-training.
	ret = DP_Link_Train_Lock(index);
	if (ret == false)
		return -1;

	return 0;
}
static int DP_Link_Train(dp_index index)
{
    int retry_count = 0;

RETRY:
    if (!DP_HPD_Detect(index))
    {
        printk("dp unplug\n");
        return DP_FAILURE;
    }

	printk("dp[%d] Training Start. rate[%x] count[%x]\n", index, g_dp_info[index].lane_rate, g_dp_info[index].lane_count);
    DP_CDR_Training(index);
    DP_EQ_Training(index);
    DP_Link_Start(index);

    if (!DP_Link_Train_Lock(index)){
        printk("link train fail\n");
        if (retry_count < 3) {
            retry_count++;
            goto RETRY;
        } else {
            if (!DP_Rate_Adjust(index))
            {
                DP_Link_Cfg(index);
                retry_count = 0;
                goto RETRY;
            }
        }
    }

	return DP_SUCCESS;
}

static int DP_Sink_Power_Ctrl(dp_index index, bool power_on)
{
    unsigned char value = 0;
    int err  = -1;

    if (g_dp_info[index].dpcd_rev < DP_REV_11)
        goto END;

    err = DP_DPCD_Read(index, DP_SET_POWER, &value, 1);
    if (err < 0)
        goto END;

    value &= ~DP_SET_POWER_MASK;
    /* Sink - (State3 Sleep):
	 * 1. Hpd asserted
	 * 2. Aux enabled for differential signal monitoring,
	 * 3. Main-link Rx disabled
	 */

	/* Sink - (State 2 standby):
	 * 1. Hpd asserted
	 * 2. Aux enabled for differential signal monitoring,
	 * 3. Main-link Rx enabled
	 */

    value |= (power_on ? DP_SET_POWER_D0 : DP_SET_POWER_D3);
    
    err = DP_DPCD_Write(index, DP_SET_POWER, &value, 1);
    if (err < 0)
        goto END;

    /* According to the DP 1.1 specification, a "Sink Device must exit the
	 * power saving state within 1 ms" (Section 2.5.3.1, Table 5-52, "Sink
	 * Control Field" (register 0x600).
	 */
	if (power_on) {
		/* For an embedded connection, a Sink device may take up to 20 ms from a power-save mode
		 * until it is ready to reply to an AUX request transaction
		 */
		if (g_dp_info[index].max_sink_rates)
			usleep_range(20000,21000);
		else
			usleep_range(1000, 1100);
	}

END:
    return err;
}

static void DP_Link_Caps_Reset(dp_index index)
{
	g_dp_info[index].cdr_delay = 0;
	g_dp_info[index].eq_delay = 0;
	g_dp_info[index].dpcd_rev = 0;
	g_dp_info[index].enhance_mode = 1;
	g_dp_info[index].max_sink_rates = 0;
	g_dp_info[index].num_sink_rates = 0;
	g_dp_info[index].lane_count = 4;
	g_dp_info[index].lane_rate = DP_LINK_BW_5_4;
	g_dp_info[index].cea_mode = 0;
	g_dp_info[index].max_downspread = 0;
}

static void DP_Compliance_Cfg(dp_index index)
{
	int ret = 0;
	unsigned int retry = 0, i = 0;

	DP_Link_Caps_Reset(index);

	for (retry = 0; retry <= 3; retry++) {
		ret = DP_DPCD_Read(index, DP_DPCD_REV, g_dp_info[index].dpcd, DP_RECEIVER_CAP_SIZE);
		if (ret == DP_RECEIVER_CAP_SIZE &&
			g_dp_info[index].dpcd[DP_DPCD_REV] >= DP_REV_10 &&
			g_dp_info[index].dpcd[DP_DPCD_REV] <= DP_REV_14) {

			g_dp_info[index].dpcd_rev = g_dp_info[index].dpcd[DP_DPCD_REV];
			g_dp_info[index].lane_rate = g_dp_info[index].dpcd[DP_MAX_LINK_RATE];
			g_dp_info[index].lane_count = g_dp_info[index].dpcd[DP_MAX_LANE_COUNT] & DP_MAX_LANE_COUNT_MASK;
			g_dp_info[index].enhance_mode = DP_Enhanced_Frame_Cap(g_dp_info[index].dpcd) ? 1 : 0;
			g_dp_info[index].cdr_delay = g_dp_info[index].dpcd[DP_TRAINING_AUX_RD_INTERVAL] & 0x7f;
			g_dp_info[index].eq_delay = g_dp_info[index].dpcd[DP_TRAINING_AUX_RD_INTERVAL] & 0x7f;

			if (g_dp_info[index].lane_rate >= DP_LINK_BW_5_4)
			{
				g_dp_info[index].lane_rate = DP_LINK_BW_5_4;
			}
			
			DP_EDP_Init_DPCD(index);

			break;
		} else if (ret < 0) {
			break;
		}

		usleep_range(5000, 5100);
	}

	/* For EDP 1.4 */
	if (g_dp_info[index].max_sink_rates)
		g_dp_info[index].lane_rate = g_dp_info[index].max_sink_rates;

	i = DP_Rate_Index(dp_bw_table, ARRAY_SIZE(dp_bw_table), g_dp_info[index].lane_rate);
	if (i >= 0) {
		g_dp_info[index].phy_rate = i;
	} else {
		g_dp_info[index].lane_rate = DP_LINK_BW_5_4;
		g_dp_info[index].phy_rate = 0x2;
	}

	if (g_dp_info[index].lane_count >= 0x1 && g_dp_info[index].lane_count <= 0x4) {
		g_dp_info[index].phy_lanes = g_dp_info[index].lane_count >> 1;
	} else {
		g_dp_info[index].phy_lanes = 0x2;
		g_dp_info[index].lane_count = 0x4;
	}

	printk("get current dp[%d] info: rev[%x] laneRate[%x] laneCount[%x] phyRate[%x] phyLanes[%x]\n", 
			index, g_dp_info[index].dpcd_rev, g_dp_info[index].lane_rate, g_dp_info[index].lane_count,
			g_dp_info[index].phy_rate, g_dp_info[index].phy_lanes);
}

static void DP_Video_Cfg(dp_index index)
{
	unsigned short htotal = g_dp_info[index].cea_mode->h_active + g_dp_info[index].cea_mode->h_blank;
	unsigned short hactive = g_dp_info[index].cea_mode->h_active;
    unsigned short vtotal = g_dp_info[index].cea_mode->v_active + g_dp_info[index].cea_mode->v_blank;
	unsigned short vactive = g_dp_info[index].cea_mode->v_active;
    unsigned short hblank = g_dp_info[index].cea_mode->h_blank;
	unsigned short hsync = g_dp_info[index].cea_mode->hsync_width;
	unsigned short hoffset = g_dp_info[index].cea_mode->h_blank - g_dp_info[index].cea_mode->hsync_delay;
	unsigned short vblank = g_dp_info[index].cea_mode->v_blank;
	unsigned short vsync = g_dp_info[index].cea_mode->vsync_width;
	unsigned short voffset = g_dp_info[index].cea_mode->v_blank - g_dp_info[index].cea_mode->vsync_delay;
    unsigned short hpolarity = g_dp_info[index].cea_mode->hsync_polarity;
    unsigned short vpolarity = g_dp_info[index].cea_mode->vsync_polarity;
	unsigned char msa_misc0 = 0x20;
	unsigned char msa_misc1 = 0x00;

	// RGB 8bit
	DP_Write32(index, DP_CONTROLLER200,
				FIELD_SET(DP_Read32(index, DP_CONTROLLER200), DP_CONTROLLER200, VIDEO_MAPPING, RGB8));
    // h/v start
	DP_Write32(index, DP_CONTROLLER224,
				FIELD_VALUE(0, DP_CONTROLLER224, REG_TX_HSTART, hoffset) |
				FIELD_VALUE(0, DP_CONTROLLER224, REG_TX_VSTART, voffset));
    // sync polarity
	DP_Write32(index, DP_CONTROLLER20C,
				FIELD_VALUE(0, DP_CONTROLLER20C, REG_TX_POLARITY_HSYNC, hpolarity) |
				FIELD_VALUE(0, DP_CONTROLLER20C, REG_TX_POLARITY_VSYNC, vpolarity));
    // misc0, 8 bits/color respectively
	DP_Write32(index, DP_CONTROLLER228,
				FIELD_VALUE(0, DP_CONTROLLER228, REG_TX_MSA_MISC0, msa_misc0));
    // h active/blank
	DP_Write32(index, DP_CONTROLLER210,
				FIELD_VALUE(0, DP_CONTROLLER210, REG_TX_HBLANK, hblank) |
				FIELD_VALUE(0, DP_CONTROLLER210, REG_TX_HACTIVE, hactive));
    // misc1
	DP_Write32(index, DP_CONTROLLER22C,
				FIELD_VALUE(0, DP_CONTROLLER22C, REG_TX_MSA_MISC1, msa_misc1));
    // v active/blank
	DP_Write32(index, DP_CONTROLLER214,
				FIELD_VALUE(0, DP_CONTROLLER214, REG_TX_VBLANK, vblank) |
				FIELD_VALUE(0, DP_CONTROLLER214, REG_TX_VACTIVE, vactive));
    // h sync/front porch
	DP_Write32(index, DP_CONTROLLER218,
				FIELD_VALUE(0, DP_CONTROLLER218, REG_TX_H_FRONT_PORCH, htotal - hactive - hoffset) |
				FIELD_VALUE(0, DP_CONTROLLER218, REG_TX_HSWIDTH, hsync));
    // v sync/front porch
	DP_Write32(index, DP_CONTROLLER21C,
				FIELD_VALUE(0, DP_CONTROLLER21C, REG_TX_V_FRONT_PORCH, vtotal - vactive - voffset) |
				FIELD_VALUE(0, DP_CONTROLLER21C, REG_TX_VSWIDTH, vsync));

	DP_Tu_Init(index);
}

static int DP_Check_EDID_Sum(unsigned char *edid)
{
	int i, checksum = 0;

	for(i = 0; i < DP_EDID_BUF_LEN; i++) // EDID_LEN -> 128
		checksum += edid[i];

	return checksum % (DP_EDID_BUF_LEN * 2); //CEA-861 Spec
}

static int DP_EDID_Is_Invalid(unsigned char *pEDIDBuffer, int blockCount)
{
	int error = 0;
	const unsigned char header[] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

	if (blockCount == 1)
	{
		error = memcmp(pEDIDBuffer, (unsigned char *)header, sizeof(header));
		if (error)
		{
			printk("EDID header check failed\n");
			goto END;
		}
	}

	error = DP_Check_EDID_Sum(pEDIDBuffer);
	if(error){
		printk("EDID checksum failed\n");
		goto END;
	}
 
END:
	return error;
}

static int DP_Read_EDID_IIC_Aux(dp_index index, unsigned char *pEDIDBuffer, unsigned short *pBufferSize, bool sigleByte)
{
	int ret = 0, blockCount = 1, i,j;
	int lenLeft = DP_EDID_BUF_LEN;
	unsigned char val[16] = { 0 };

	/* Send a bare address packet to start the transaction.
	 * Zero sized messages specify an address only (bare
	 * address) transaction.
	 */
	DP_Aux_Transfer(index, DP_AUX_I2C_WRITE, 0x50, NULL, 0);

	for (i = 0; i < blockCount; i++)
	{
		lenLeft = DP_EDID_BUF_LEN;
		do
		{
			if (lenLeft < DP_AUX_MAX_PAYLOAD_BYTES || sigleByte)
			{
				ret = DP_Aux_Transfer(index, DP_AUX_I2C_READ, 0x50, val, 1);
				if (ret <= 0 || ret > DP_AUX_MAX_PAYLOAD_BYTES)
					goto ERR;
			}else{
				ret = DP_Aux_Transfer(index, DP_AUX_I2C_READ, 0x50, val, DP_AUX_MAX_PAYLOAD_BYTES);
				if (ret <= 0 || ret > DP_AUX_MAX_PAYLOAD_BYTES)
					goto ERR;
			}

			for (j = 0; j < ret; j++)
				pEDIDBuffer[j + *pBufferSize] = val[j];

			*pBufferSize += ret;
			lenLeft = DP_EDID_BUF_LEN * (i + 1) - *pBufferSize;
		} while (lenLeft > 0);

		blockCount = pEDIDBuffer[126] + 1;
		if (*pBufferSize % DP_EDID_BUF_LEN != 0)
			goto ERR;
		if (DP_EDID_Is_Invalid(pEDIDBuffer, *pBufferSize / DP_EDID_BUF_LEN))
			goto ERR;
	}

	/* Send a bare address packet to close out the transaction.
	 * Zero sized messages specify an address only (bare
	 * address) transaction.
	 */
	DP_Aux_Transfer(index, DP_AUX_I2C_WRITE & ~DP_AUX_I2C_MOT, 0x50, NULL, 0);
	return DP_SUCCESS;
ERR:
	DP_Aux_Transfer(index, DP_AUX_I2C_WRITE & ~DP_AUX_I2C_MOT, 0x50, NULL, 0);
	return DP_FAILURE;
}

void DP_Set_Channel(dp_index index, disp_control_t dc)
{
    disp_output_t disp_index;

    if (index == INDEX_DP0)
    {
        disp_index = DP0;
    }else if (index == INDEX_DP1){
        disp_index = DP1;
    }
    
    setDCMUX(disp_index, dc);
}

void DP_Clear_Channel(dp_index index)
{
    disp_output_t disp_index;

    if (index == INDEX_DP0)
    {
        disp_index = DP0;
    }else if (index == INDEX_DP1){
        disp_index = DP1;
    }
    
    ClearDCMUX(disp_index);
}

unsigned char DP_Get_Channel(dp_index index)
{
    disp_output_t disp_index;

    if (index == INDEX_DP0)
    {
        disp_index = DP0;
    }else if (index == INDEX_DP1){
        disp_index = DP1;
    }
    
    return GetDCMUX(disp_index);
}

static void DP_HPD_Enable(dp_index index)
{
	unsigned int regval = 0;
	
	/*fast sim*/
	DP_Write32(index, DP_TOP18,
				FIELD_SET(DP_Read32(index, DP_TOP18), DP_TOP18, REG_HPD_SCALE, SCALING));

	regval = DP_Read32(index, DP_TOP84);
	// enable plug in hpd event
	regval = FIELD_SET(regval, DP_TOP84, ENABLE_HPD_PLUG_EVENT, TRUE);
	// enable unplug hpd event
	regval = FIELD_SET(regval, DP_TOP84, ENABLE_HPD_UNPLUG_EVENT, TRUE);
	DP_Write32(index, DP_TOP84, regval);

	regval = DP_Read32(index, DP_TOP8C);		
    // enable hpd plug
	regval = FIELD_SET(regval, DP_TOP8C, HPD_PLUG, ENABLE);
	// enable hpd unplug
	regval = FIELD_SET(regval, DP_TOP8C, HPD_UNPLUG, ENABLE);
	// enable hpd irq	
	regval = FIELD_SET(regval, DP_TOP8C, HPD_IRQ, ENABLE);
	DP_Write32(index, DP_TOP8C, regval);
}

int DP_HPD_Detect(dp_index index)
{
    DP_HPD_Enable(index);

	if ((DP_Read32(index, DP_TOP88) & 0xf00) == 0xf00)
			return true;

	return false;
}

int DP_Enable(dp_index index)
{
    DP_Sink_Power_Ctrl(index, true);

    if (DP_Link_Train(index) == DP_FAILURE)
	{
		return DP_FAILURE;
	}

    DP_Write32(index, DP_CONTROLLER200, 
                FIELD_SET(DP_Read32(index, DP_CONTROLLER200), DP_CONTROLLER200, VIDEO_STREAM, ENABLE));

	DP_Audio_Start(index, DP_CONTROLLER300_AUDIO_DATA_WIDTH_24, 2);

	DP_Audio_CheckOverrun(index);
	
	return DP_SUCCESS;
}

int DP_Disable(dp_index index)
{
	DP_Write32(index, DP_PHY100, 
				FIELD_SET(DP_Read32(index, DP_PHY100), DP_PHY100, TRANSMIT_ENABLE, NONE));
	
	DP_Write32(index, DP_CONTROLLER200,
				FIELD_SET(DP_Read32(index, DP_CONTROLLER200), DP_CONTROLLER200, VIDEO_STREAM, DISABLE));

	return 0;
}


void DP_SSC_Enable(dp_index index, unsigned int downspread, unsigned int freq, unsigned int amp)
{
	unsigned int val;

	printk("Enable DP SSC\n");

	val = DP_Read32(index, DP_ANALOG188);
	if (freq){
		freq = 24000000/pll_table[g_dp_info[index].phy_rate][pll_fbdiv]/128/freq;
		val = FIELD_VALUE(val, DP_ANALOG188, REG_DIVVAL, freq);
	}
	else
		val = FIELD_VALUE(val, DP_ANALOG188, REG_DIVVAL, 0x3);
	
	if (amp)
		val = FIELD_VALUE(val, DP_ANALOG188, REG_SPREAD, amp);
	DP_Write32(index, DP_ANALOG188, val);

	val = DP_Read32(index, DP_ANALOG180);

	if (downspread)
		val = FIELD_SET(val, DP_ANALOG180, DOWNSPREAD, DOWN);
	else
		val = FIELD_SET(val, DP_ANALOG180, DOWNSPREAD, CENTER);

	val = FIELD_VALUE(val, DP_ANALOG180, DA_COREPLL_FRAC_PD, 0X0);
	val = FIELD_SET(val, DP_ANALOG180, SSCG, ENABLE);
	DP_Write32(index, DP_ANALOG180, val);
}

void DP_SSC_Disable(dp_index index)
{

	DP_Write32(index, DP_ANALOG180, 
		FIELD_SET(DP_Read32(index, DP_ANALOG180), DP_ANALOG180, SSCG, DISABLE));	

}

int DP_Setmode(dp_index index, logicalMode_t *pLogicalMode , mode_parameter_t *pModeParam)
{
    cea_parameter_t cea_mode = {0};
	unsigned int offset = (pLogicalMode->dispCtrl > 1)? CHANNEL_OFFSET2 : pLogicalMode->dispCtrl * CHANNEL_OFFSET;

	if (!DP_HPD_Detect(index))
    {
        printk("dp unplug\n");
        return DP_FAILURE;
    }

	printk("DP Setmode Start.\n");



    if ((int)Get_CEA_Mode(pLogicalMode,&cea_mode, pModeParam, 1) < 0)
    {
        return DP_FAILURE;
    }

    DP_Drv_Init(index);
    DP_Aux_Init(index);

    DP_Compliance_Cfg(index);
	g_dp_info[index].cea_mode = &cea_mode;
	g_dp_info[index].sink_power_status = 1;

    DP_Link_Cfg(index);
	
    DP_Write32(index, DP_TOP18, 
                FIELD_SET(DP_Read32(index, DP_TOP18), DP_TOP18, SCRAMBLE, ENABLE));
    DP_Write32(index, DP_TOP18, 
                FIELD_SET(DP_Read32(index, DP_TOP18), DP_TOP18, ENHANCE_FRAMING, ENABLE));
    DP_Write32(index, DP_TOP18, 
                FIELD_SET(DP_Read32(index, DP_TOP18), DP_TOP18, REG_HPD_SCALE, SCALING));

    pokeRegisterDWord(VERTICAL_SYNC + offset, FIELD_VALUE(peekRegisterDWord(VERTICAL_SYNC + offset), VERTICAL_SYNC, DPMODE, 0));

    DP_Video_Cfg(index);

	if (DP_Enable(index) == DP_FAILURE)
	{

		return DP_FAILURE;
	}


	printk("DP Setmode Finish.\n");
    return DP_SUCCESS;
}

__attribute__((unused)) static unsigned char Check_Sink_Power(dp_index index)
{
	return g_dp_info[index].sink_power_status;
}

void DP_Enable_Output(dp_index index)
{
	DP_Write32(index, DP_CONTROLLER200, 
            FIELD_SET(DP_Read32(index, DP_CONTROLLER200),DP_CONTROLLER200, VIDEO_STREAM, ENABLE));
	DP_Audio_UnMute(index);
}

void DP_Disable_Output(dp_index index)
{
	DP_Write32(index, DP_CONTROLLER200, 
        FIELD_SET(DP_Read32(index, DP_CONTROLLER200),DP_CONTROLLER200, VIDEO_STREAM,DISABLE));
	DP_Audio_Mute(index);
}

int DP_Read_EDID(dp_index index, unsigned char *pEDIDBuffer, unsigned short *pBufferSize)
{
	int ret = -1, retry;
	unsigned char *pTmpaddr = pEDIDBuffer;
	*pBufferSize = 0;


	if (pEDIDBuffer == NULL)
    {
        printk("buffer is NULL!\n");
        return 0;
    }

	if (DP_HPD_Detect(index))
	{
		bool singleByte = false;
		for (retry = 0; retry < 5; retry++)
		{
			if (retry >=2)
				singleByte = true;
			if (DP_Read_EDID_IIC_Aux(index, pEDIDBuffer, pBufferSize, singleByte) == DP_SUCCESS)
			{
				return 0;
			}
			pEDIDBuffer = pTmpaddr;
			*pBufferSize = 0;
		}
	}

	return ret;
}

/*
The GPIO needs to be forcibly pulled low 
to avoid the HPD state remaining as "connected" when the DP HPD pin is floating.
*/
static void DP_HPD_PD_Enable(dp_index index)
{
	unsigned int regval;
	regval = peekRegisterDWord(GPIO_PAD_DP0_HPD + index * 4);
	regval = FIELD_SET(regval, GPIO_PAD_DP0_HPD, PD, ENABLE);

	pokeRegisterDWord(GPIO_PAD_DP0_HPD + index * 4, regval);
}
int DP_Init(dp_index index)
{
	// From INNO advice
	DP_Write32(index, 0x1a4 , 0x55ff0f0f);
	DP_Write32(index, 0x1a8 , 0x1f1f0055);
	DP_Write32(index, 0x1ac , 0x44441f1f);
	DP_Write32(index, 0x1b0 , 0xf0000000);
	DP_Write32(index, 0x1a0 , 0x0000d090);

    DP_Drv_Init(index);
    DP_Aux_Init(index);
	DP_HPD_PD_Enable(index);
    return DP_SUCCESS;
}

//-----------------------------------------------------------------------------
// DP Interrupt functions
//-----------------------------------------------------------------------------
typedef struct _dp_interrupt_t
{
	struct _dp_interrupt_t *next;
	void (*handler)(void);
} dp_interrupt_t;

static dp_interrupt_t *g_pDP0IntHandlers = ((dp_interrupt_t *)0);
static dp_interrupt_t *g_pDP1IntHandlers = ((dp_interrupt_t *)0);

/* HDMI Interrupt Service Routine */
__attribute__((unused)) static void dp0ISR(
	unsigned int status)
{
	dp_interrupt_t *pfnHandler;

	if (FIELD_VAL_GET(status, INT_STATUS, DP0) == INT_STATUS_DP0_ACTIVE)
	{
		/* Walk all registered handlers for handlers that support this interrupt status */
		for (pfnHandler = g_pDP0IntHandlers; pfnHandler != ((dp_interrupt_t *)0); pfnHandler = pfnHandler->next)
			pfnHandler->handler();

		// What we need to do in ISR
		printk("We are in DP0 ISR\n");
	}
}

__attribute__((unused)) static void dp1ISR(
	unsigned int status)
{
	dp_interrupt_t *pfnHandler;

	if (FIELD_VAL_GET(status, INT_STATUS, DP1) == INT_STATUS_DP1_ACTIVE)
	{
		/* Walk all registered handlers for handlers that support this interrupt status */
		for (pfnHandler = g_pDP1IntHandlers; pfnHandler != ((dp_interrupt_t *)0); pfnHandler = pfnHandler->next)
			pfnHandler->handler();

		// What we need to do in ISR
		printk("We are in DP1 ISR\n");
	}
}

void DP_Hpd_Interrupt_Enable(dp_index index, unsigned int enable)
{
	unsigned int DPBaseAddr, value;

	DPBaseAddr = DP_BASE + (DP_OFFSET * index);

	if (enable)
	{
		value = peekRegisterDWord(INT_MASK);
		if (index == INDEX_DP0)
			value = FIELD_SET(value, INT_MASK, DP0, ENABLE);
		else
			value = FIELD_SET(value, INT_MASK, DP1, ENABLE);
		pokeRegisterDWord(INT_MASK, value);

		DP_HPD_Enable(index);
	}
	else
	{
		value = peekRegisterDWord(INT_MASK);
		if (index == INDEX_DP0)
			value = FIELD_SET(value, INT_MASK, DP0, DISABLE);
		else
			value = FIELD_SET(value, INT_MASK, DP1, DISABLE);
		pokeRegisterDWord(INT_MASK, value);

		// hpd dectection
		value = FIELD_SET(peekRegisterDWord(DPBaseAddr + DP_TOP84), DP_TOP84, ENABLE_HPD_PLUG_EVENT, FALSE);
		value = FIELD_SET(value, DP_TOP84, ENABLE_HPD_UNPLUG_EVENT, FALSE);
		pokeRegisterDWord(DPBaseAddr + DP_TOP84, value); // disable hpd event

		value = FIELD_SET(peekRegisterDWord(DPBaseAddr + DP_TOP8C), DP_TOP8C, HPD_PLUG, DISABLE);
		value = FIELD_SET(value, DP_TOP8C, HPD_UNPLUG, DISABLE);
		value = FIELD_SET(value, DP_TOP8C, HPD_IRQ, DISABLE);	
		pokeRegisterDWord(DPBaseAddr + DP_TOP8C, value); // disable hpd plug
	}
}

#if USE_I2C_ADAPTER
static void drm_dp_i2c_msg_set_request(struct drm_dp_aux_msg *msg,
				       const struct i2c_msg *i2c_msg)
{
	msg->request = (i2c_msg->flags & I2C_M_RD) ?
		DP_AUX_I2C_READ : DP_AUX_I2C_WRITE;
	if (!(i2c_msg->flags & I2C_M_STOP))
		msg->request |= DP_AUX_I2C_MOT;
}

static int ddk770_dp_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct smi_connector *connector = i2c_get_adapdata(adap);
	struct drm_dp_aux_msg msg;
	int i, j, index;
	int ret = 0;
	unsigned transfer_size;
	u8 addr = msgs[0].addr;

	if (addr == DDC_CI_ADDR)
		/*
		 * The internal I2C controller does not support the multi-byte
		 * read and write operations needed for DDC/CI.
		 * TOFIX: Blacklist the DDC/CI address until we filter out
		 * unsupported I2C operations.
		 */
		return -EOPNOTSUPP;


	mutex_lock(&connector->i2c_lock);
	memset(&msg, 0, sizeof(msg));

	connector->i2c_is_segment = false;
	connector->i2c_is_regaddr = false;

	/* reset */
	if (connector->base.connector_type == DRM_MODE_CONNECTOR_DisplayPort) {
		index = 0;
	} else if (connector->base.connector_type == DRM_MODE_CONNECTOR_eDP) {
		index = 1;
	} else {
		pr_err("Unknown connector type\n");
		ret = -EMEDIUMTYPE;
		goto end;
	}

	if (!DP_HPD_Detect(index)) {
		ret = -ENODEV;
		goto end;
	}

	//DP_Aux_Transfer(index, DP_AUX_I2C_WRITE & ~DP_AUX_I2C_MOT, 0x50, 0x0,1);
	for (i = 0; i < num; i++) {
		if ((!(msgs[i].flags & I2C_M_RD)) && msgs[i].len == 1 &&
		    msgs[i].buf[0] == 0x00) {
			connector->i2c_is_segment = true;
		}

		if ((msgs[i].flags & I2C_M_RD) && (msgs[i].len > 1) &&
		    (connector->i2c_is_segment == true)) {
			connector->i2c_is_regaddr = true;
		}

		if (connector->i2c_is_regaddr == true) {
			DP_Aux_Transfer(index,
					DP_AUX_I2C_WRITE & ~DP_AUX_I2C_MOT,
					0x50, NULL, 0);
		}

		msg.address = msgs[i].addr;
		drm_dp_i2c_msg_set_request(&msg, &msgs[i]);

		msg.buffer = NULL;
		msg.size = 0;

		transfer_size = 16;
		for (j = 0; j < msgs[i].len; j += msg.size) {

			msg.buffer = msgs[i].buf + j;
			if (msgs[i].len - j > transfer_size) {
				msg.size = transfer_size;
			} else {
				msg.size = msgs[i].len - j;
			}

			ret = DP_Aux_Transfer(index, msg.request, msg.address,
					      msg.buffer, msg.size);
			/*
			 * Reset msg.request in case in case it got
			 * changed into a WRITE_STATUS_UPDATE.
			 */
			// drm_dp_i2c_msg_set_request(&msg, &msgs[i]);

			if (ret < 0) {
				pr_err("[error] ddk770_hdmi_i2c_xfer   ret:%d\n",ret);
				goto end;
			}
			transfer_size = ret;
		}
		DP_Aux_Transfer(index, DP_AUX_I2C_WRITE & ~DP_AUX_I2C_MOT, 0x50, NULL,0);
	}


	ret = num;
	mutex_unlock(&connector->i2c_lock);
	return ret;

end:
	mutex_unlock(&connector->i2c_lock);
	return ret;
}

static u32 ddk770_dp_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm ddk770_dp_i2c_algo = {
	.master_xfer = ddk770_dp_i2c_xfer,
	.functionality = ddk770_dp_i2c_func
};

long ddk770_DP_AdaptHWI2CInit(struct smi_connector *connector)
{
	//TODO
	int ret = 0;

	mutex_init(&connector->i2c_lock);

	connector->dp_adapter.owner = THIS_MODULE;
	snprintf(connector->dp_adapter.name, I2C_NAME_SIZE, "SMI HW DP I2C Bus");
	connector->dp_adapter.dev.parent = connector->base.dev->dev;
	i2c_set_adapdata(&connector->dp_adapter, connector);
	connector->dp_adapter.algo = &ddk770_dp_i2c_algo;
	ret = i2c_add_adapter(&connector->dp_adapter);
	if (ret) {
		pr_err("HW i2c add dp_adapter failed. %d\n", ret);
		return -1;
	}

	return ret;
}
#endif