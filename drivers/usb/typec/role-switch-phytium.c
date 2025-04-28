// SPDX-License-Identifier: GPL-2.0-only

/*
 * Driver for USB Type-C and PD controller
 *
 * Copyright (C) 2021-2023, Phytium Technology Co., Ltd.
 *
 */
#include <linux/acpi.h>
#include <acpi/acpi_bus.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_graph.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/usb/pd.h>
#include <linux/usb/role.h>
#include <linux/usb/tcpci.h>
#include <linux/usb/typec.h>
#include <linux/usb/typec_dp.h>
#include <linux/usb/typec_mux.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>

#define RS_PHYTIUM_DRV_VERSION			"1.0.0"

#define RS_PHYTIUM_TCPC_ADDRESS1		0x58
#define RS_PHYTIUM_TCPC_ADDRESS2		0x56
#define RS_PHYTIUM_TCPC_ADDRESS3		0x54
#define RS_PHYTIUM_TCPC_ADDRESS4		0x52
#define RS_PHYTIUM_SPI_ADDRESS1		0x7e
#define RS_PHYTIUM_SPI_ADDRESS2		0x6e
#define RS_PHYTIUM_SPI_ADDRESS3		0x64
#define RS_PHYTIUM_SPI_ADDRESS4		0x62

struct role_sw_phytium_i2c_select {
	u8 tcpc_address;
	u8 spi_address;
};

#define RS_PHYTIUM_VID_ANALOGIX		0x1F29
#define RS_PHYTIUM_PID_ANALOGIX		0x7411

/* TCPC register define */

#define RS_PHYTIUM_ANALOG_CTRL_10		0xAA

#define RS_PHYTIUM_STATUS_LEN		2
#define RS_PHYTIUM_ALERT_0			0xCB
#define RS_PHYTIUM_RECEIVED_MSG		BIT(7)
#define RS_PHYTIUM_SOFTWARE_INT		BIT(6)
#define RS_PHYTIUM_MSG_LEN			32
#define RS_PHYTIUM_HEADER_LEN		2
#define RS_PHYTIUM_MSG_HEADER		0x00
#define RS_PHYTIUM_MSG_TYPE		0x01
#define RS_PHYTIUM_MSG_RAWDATA		0x02
#define RS_PHYTIUM_MSG_LEN_MASK		0x1F

#define RS_PHYTIUM_ALERT_1			0xCC
#define RS_PHYTIUM_INTP_POW_ON		BIT(7)
#define RS_PHYTIUM_INTP_POW_OFF		BIT(6)

#define RS_PHYTIUM_VBUS_THRESHOLD_H	0xDD
#define RS_PHYTIUM_VBUS_THRESHOLD_L	0xDE

#define RS_PHYTIUM_FW_CTRL_0		0xF0
#define RS_PHYTIUM_UNSTRUCT_VDM_EN		BIT(0)
#define RS_PHYTIUM_DELAY_200MS		BIT(1)
#define RS_PHYTIUM_VSAFE0			0
#define RS_PHYTIUM_VSAFE1			BIT(2)
#define RS_PHYTIUM_VSAFE2			BIT(3)
#define RS_PHYTIUM_VSAFE3			(BIT(2) | BIT(3))
#define RS_PHYTIUM_FRS_EN			BIT(7)

#define RS_PHYTIUM_FW_PARAM		0xF1
#define RS_PHYTIUM_DONGLE_IOP		BIT(0)

#define RS_PHYTIUM_FW_CTRL_2		0xF7
#define RS_PHYTIUM_SINK_CTRL_DIS_FLAG	BIT(5)

/* SPI register define */
#define RS_PHYTIUM_OCM_CTRL_0		0x6E
#define RS_PHYTIUM_OCM_RESET		BIT(6)

#define RS_PHYTIUM_MAX_VOLTAGE		0xAC
#define RS_PHYTIUM_MAX_POWER		0xAD
#define RS_PHYTIUM_MIN_POWER		0xAE

#define RS_PHYTIUM_REQUEST_VOLTAGE		0xAF
#define RS_PHYTIUM_VOLTAGE_UNIT		100 /* mV per unit */

#define RS_PHYTIUM_REQUEST_CURRENT		0xB1
#define RS_PHYTIUM_CURRENT_UNIT		50 /* mA per unit */

#define RS_PHYTIUM_CMD_SEND_BUF		0xC0
#define RS_PHYTIUM_CMD_RECV_BUF		0xE0

#define RS_PHYTIUM_REQ_VOL_20V_IN_100MV	0xC8
#define RS_PHYTIUM_REQ_CUR_2_25A_IN_50MA	0x2D
#define RS_PHYTIUM_REQ_CUR_3_25A_IN_50MA	0x41

#define RS_PHYTIUM_DEF_5V			5000
#define RS_PHYTIUM_DEF_1_5A		1500

#define RS_PHYTIUM_LOBYTE(w)		((u8)((w) & 0xFF))
#define RS_PHYTIUM_HIBYTE(w)		((u8)(((u16)(w) >> 8) & 0xFF))

enum role_sw_phytium_typec_message_type {
	RS_PHYTIUM_TYPE_SRC_CAP = 0x00,
	RS_PHYTIUM_TYPE_SNK_CAP = 0x01,
	RS_PHYTIUM_TYPE_SNK_IDENTITY = 0x02,
	RS_PHYTIUM_TYPE_SVID = 0x03,
	RS_PHYTIUM_TYPE_SET_SNK_DP_CAP = 0x08,
	RS_PHYTIUM_TYPE_PSWAP_REQ = 0x10,
	RS_PHYTIUM_TYPE_DSWAP_REQ = 0x11,
	RS_PHYTIUM_TYPE_VDM = 0x14,
	RS_PHYTIUM_TYPE_OBJ_REQ = 0x16,
	RS_PHYTIUM_TYPE_DP_ALT_ENTER = 0x19,
	RS_PHYTIUM_TYPE_DP_DISCOVER_MODES_INFO = 0x27,
	RS_PHYTIUM_TYPE_GET_DP_CONFIG = 0x29,
	RS_PHYTIUM_TYPE_DP_CONFIGURE = 0x2A,
	RS_PHYTIUM_TYPE_GET_DP_DISCOVER_MODES_INFO = 0x2E,
	RS_PHYTIUM_TYPE_GET_DP_ALT_ENTER = 0x2F,
};

#define RS_PHYTIUM_FW_CTRL_1		0xB2
#define RS_PHYTIUM_AUTO_PD_EN		BIT(1)
#define RS_PHYTIUM_TRYSRC_EN		BIT(2)
#define RS_PHYTIUM_TRYSNK_EN		BIT(3)
#define RS_PHYTIUM_FORCE_SEND_RDO		BIT(6)

#define RS_PHYTIUM_FW_VER			0xB4
#define RS_PHYTIUM_FW_SUBVER		0xB5

#define RS_PHYTIUM_INT_MASK		0xB6
#define RS_PHYTIUM_INT_STS			0xB7
#define RS_PHYTIUM_OCM_BOOT_UP		BIT(0)
#define RS_PHYTIUM_OC_OV_EVENT		BIT(1)
#define RS_PHYTIUM_VCONN_CHANGE		BIT(2)
#define RS_PHYTIUM_VBUS_CHANGE		BIT(3)
#define RS_PHYTIUM_CC_STATUS_CHANGE	BIT(4)
#define RS_PHYTIUM_DATA_ROLE_CHANGE	BIT(5)
#define RS_PHYTIUM_PR_CONSUMER_GOT_POWER	BIT(6)
#define RS_PHYTIUM_HPD_STATUS_CHANGE	BIT(7)

#define RS_PHYTIUM_SYSTEM_STSTUS		0xB8
/* 0: SINK off; 1: SINK on */
#define RS_PHYTIUM_SINK_STATUS		BIT(1)
/* 0: VCONN off; 1: VCONN on*/
#define RS_PHYTIUM_VCONN_STATUS		BIT(2)
/* 0: vbus off; 1: vbus on*/
#define RS_PHYTIUM_VBUS_STATUS		BIT(3)
/* 1: host; 0:device*/
#define RS_PHYTIUM_DATA_ROLE		BIT(5)
/* 0: Chunking; 1: Unchunked*/
#define RS_PHYTIUM_SUPPORT_UNCHUNKING	BIT(6)
/* 0: HPD low; 1: HPD high*/
#define RS_PHYTIUM_HPD_STATUS		BIT(7)

#define RS_PHYTIUM_DATA_DFP		1
#define RS_PHYTIUM_DATA_UFP		2
#define RS_PHYTIUM_POWER_SOURCE		1
#define RS_PHYTIUM_POWER_SINK		2

#define RS_PHYTIUM_CC_STATUS		0xB9
#define RS_PHYTIUM_CC1_RD			BIT(0)
#define RS_PHYTIUM_CC2_RD			BIT(4)
#define RS_PHYTIUM_CC1_RA			BIT(1)
#define RS_PHYTIUM_CC2_RA			BIT(5)
#define RS_PHYTIUM_CC1_RD			BIT(0)
#define RS_PHYTIUM_CC1_RP(cc)		(((cc) >> 2) & 0x03)
#define RS_PHYTIUM_CC2_RP(cc)		(((cc) >> 6) & 0x03)

#define RS_PHYTIUM_PD_REV_INIT		0xBA

#define RS_PHYTIUM_PD_EXT_MSG_CTRL		0xBB
#define RS_PHYTIUM_SRC_CAP_EXT_REPLY	BIT(0)
#define RS_PHYTIUM_MANUFACTURER_INFO_REPLY	BIT(1)
#define RS_PHYTIUM_BATTERY_STS_REPLY	BIT(2)
#define RS_PHYTIUM_BATTERY_CAP_REPLY	BIT(3)
#define RS_PHYTIUM_ALERT_REPLY		BIT(4)
#define RS_PHYTIUM_STATUS_REPLY		BIT(5)
#define RS_PHYTIUM_PPS_STATUS_REPLY	BIT(6)
#define RS_PHYTIUM_SNK_CAP_EXT_REPLY	BIT(7)

#define RS_PHYTIUM_NO_CONNECT		0x00
#define RS_PHYTIUM_USB3_1_CONNECTED	0x01
#define RS_PHYTIUM_DP_ALT_4LANES		0x02
#define RS_PHYTIUM_USB3_1_DP_2LANES	0x03
#define RS_PHYTIUM_CC1_CONNECTED		0x01
#define RS_PHYTIUM_CC2_CONNECTED		0x02
#define RS_PHYTIUM_SELECT_PIN_ASSIGMENT_C	0x04
#define RS_PHYTIUM_SELECT_PIN_ASSIGMENT_D	0x08
#define RS_PHYTIUM_SELECT_PIN_ASSIGMENT_E	0x10
#define RS_PHYTIUM_SELECT_PIN_ASSIGMENT_U	0x00
#define RS_PHYTIUM_REDRIVER_ADDRESS	0x20
#define RS_PHYTIUM_REDRIVER_OFFSET		0x00

#define RS_PHYTIUM_DP_SVID			0xFF01
#define RS_PHYTIUM_VDM_ACK			0x40
#define RS_PHYTIUM_VDM_CMD_RES		0x00
#define RS_PHYTIUM_VDM_CMD_DIS_ID		0x01
#define RS_PHYTIUM_VDM_CMD_DIS_SVID	0x02
#define RS_PHYTIUM_VDM_CMD_DIS_MOD		0x03
#define RS_PHYTIUM_VDM_CMD_ENTER_MODE	0x04
#define RS_PHYTIUM_VDM_CMD_EXIT_MODE	0x05
#define RS_PHYTIUM_VDM_CMD_ATTENTION	0x06
#define RS_PHYTIUM_VDM_CMD_GET_STS		0x10
#define RS_PHYTIUM_VDM_CMD_AND_ACK_MASK	0x5F

#define RS_PHYTIUM_MAX_ALTMODE		2

#define RS_PHYTIUM_HAS_SOURCE_CAP		BIT(0)
#define RS_PHYTIUM_HAS_SINK_CAP		BIT(1)
#define RS_PHYTIUM_HAS_SINK_WATT		BIT(2)

enum role_sw_phytium_psy_state {
	/* copy from drivers/usb/typec/tcpm */
	RS_PHYTIUM_PSY_OFFLINE = 0,
	RS_PHYTIUM_PSY_FIXED_ONLINE,

	/* private */
	/* PD keep in, but disconnct power to bq25700,
	 * this state can be active when higher capacity adapter plug in,
	 * and change to ONLINE state when higher capacity adapter plug out
	 */
	RS_PHYTIUM_PSY_HANG = 0xff,
};

struct role_sw_phytium_typec_params {
	int request_current; /* ma */
	int request_voltage; /* mv */
	int cc_connect;
	int cc_orientation_valid;
	int cc_status;
	int data_role;
	int power_role;
	int vconn_role;
	int dp_altmode_enter;
	int cust_altmode_enter;
	struct usb_role_switch *role_sw;
	struct typec_port *port;
	struct typec_partner *partner;
	struct typec_mux_dev *typec_mux;
	struct typec_switch_dev *typec_switch;
	struct typec_altmode *amode[RS_PHYTIUM_MAX_ALTMODE];
	struct typec_altmode *port_amode[RS_PHYTIUM_MAX_ALTMODE];
	struct typec_displayport_data data;
	int pin_assignment;
	struct typec_capability caps;
	u32 src_pdo[PDO_MAX_OBJECTS];
	u32 sink_pdo[PDO_MAX_OBJECTS];
	u8 caps_flags;
	u8 src_pdo_nr;
	u8 sink_pdo_nr;
	u8 sink_watt;
	u8 sink_voltage;
};

#define RS_PHYTIUM_MAX_BUF_LEN	30
struct role_sw_phytium_fw_msg {
	u8 msg_len;
	u8 msg_type;
	u8 buf[RS_PHYTIUM_MAX_BUF_LEN];
} __packed;

struct role_sw_phytium_data {
	int fw_version;
	int fw_subversion;
	struct i2c_client *tcpc_client;
	struct i2c_client *spi_client;
	struct role_sw_phytium_fw_msg send_msg;
	struct role_sw_phytium_fw_msg recv_msg;
	struct gpio_desc *intp_gpiod;
	struct fwnode_handle *connector_fwnode;
	struct acpi_device *adev;
	struct role_sw_phytium_typec_params typec;
	int intp_irq;
	struct work_struct work;
	struct workqueue_struct *workqueue;
	/* Lock for interrupt work queue */
	struct mutex lock;

	enum role_sw_phytium_psy_state psy_online;
	enum power_supply_usb_type usb_type;
	struct power_supply *psy;
	struct power_supply_desc psy_desc;
	struct device *dev;
};

static u8 snk_identity[] = {
	RS_PHYTIUM_LOBYTE(RS_PHYTIUM_VID_ANALOGIX),
	RS_PHYTIUM_HIBYTE(RS_PHYTIUM_VID_ANALOGIX), 0x00, 0x82, /* snk_id_hdr */
	0x00, 0x00, 0x00, 0x00,                                 /* snk_cert */
	0x00, 0x00, RS_PHYTIUM_LOBYTE(RS_PHYTIUM_PID_ANALOGIX),
	RS_PHYTIUM_HIBYTE(RS_PHYTIUM_PID_ANALOGIX), /* 5snk_ama */
};

static u8 dp_caps[4] = {0xC6, 0x00, 0x00, 0x00};

static int role_sw_phytium_reg_read(struct i2c_client *client,
			    u8 reg_addr)
{
	return i2c_smbus_read_byte_data(client, reg_addr);
}

static int role_sw_phytium_reg_block_read(struct i2c_client *client,
				  u8 reg_addr, u8 len, u8 *buf)
{
	return i2c_smbus_read_i2c_block_data(client, reg_addr, len, buf);
}

static int role_sw_phytium_reg_write(struct i2c_client *client,
			     u8 reg_addr, u8 reg_val)
{
	return i2c_smbus_write_byte_data(client, reg_addr, reg_val);
}

static int role_sw_phytium_reg_block_write(struct i2c_client *client,
				   u8 reg_addr, u8 len, u8 *buf)
{
	return i2c_smbus_write_i2c_block_data(client, reg_addr, len, buf);
}

static struct role_sw_phytium_i2c_select role_sw_phytium_i2c_addr[] = {
	{RS_PHYTIUM_TCPC_ADDRESS1, RS_PHYTIUM_SPI_ADDRESS1},
	{RS_PHYTIUM_TCPC_ADDRESS2, RS_PHYTIUM_SPI_ADDRESS2},
	{RS_PHYTIUM_TCPC_ADDRESS3, RS_PHYTIUM_SPI_ADDRESS3},
	{RS_PHYTIUM_TCPC_ADDRESS4, RS_PHYTIUM_SPI_ADDRESS4},
};

static int role_sw_phytium_detect_power_mode(struct role_sw_phytium_data *ctx)
{
	int ret;
	int mode;

	ret = role_sw_phytium_reg_read(ctx->spi_client, RS_PHYTIUM_REQUEST_CURRENT);
	if (ret < 0)
		return ret;

	ctx->typec.request_current = ret * RS_PHYTIUM_CURRENT_UNIT; /* 50ma per unit */

	ret = role_sw_phytium_reg_read(ctx->spi_client, RS_PHYTIUM_REQUEST_VOLTAGE);
	if (ret < 0)
		return ret;

	ctx->typec.request_voltage = ret * RS_PHYTIUM_VOLTAGE_UNIT; /* 100mv per unit */

	if (ctx->psy_online == RS_PHYTIUM_PSY_OFFLINE) {
		ctx->psy_online = RS_PHYTIUM_PSY_FIXED_ONLINE;
		ctx->usb_type = POWER_SUPPLY_USB_TYPE_PD;
		power_supply_changed(ctx->psy);
	}

	if (!ctx->typec.cc_orientation_valid)
		return 0;

	if (ctx->typec.cc_connect == RS_PHYTIUM_CC1_CONNECTED)
		mode = RS_PHYTIUM_CC1_RP(ctx->typec.cc_status);
	else
		mode = RS_PHYTIUM_CC2_RP(ctx->typec.cc_status);
	if (mode) {
		typec_set_pwr_opmode(ctx->typec.port, mode - 1);
		return 0;
	}

	typec_set_pwr_opmode(ctx->typec.port, TYPEC_PWR_MODE_PD);

	return 0;
}

static int role_sw_phytium_register_partner(struct role_sw_phytium_data *ctx,
				    int pd, int accessory)
{
	struct typec_partner_desc desc;
	struct typec_partner *partner;

	if (ctx->typec.partner)
		return 0;

	desc.usb_pd = pd;
	desc.accessory = accessory;
	desc.identity = NULL;
	partner = typec_register_partner(ctx->typec.port, &desc);
	if (IS_ERR(partner))
		return PTR_ERR(partner);

	ctx->typec.partner = partner;

	return 0;
}

static int role_sw_phytium_detect_cc_orientation(struct role_sw_phytium_data *ctx)
{
	struct device *dev = &ctx->spi_client->dev;
	int ret;
	int cc1_rd, cc2_rd;
	int cc1_ra, cc2_ra;
	int cc1_rp, cc2_rp;

	ret = role_sw_phytium_reg_read(ctx->spi_client, RS_PHYTIUM_CC_STATUS);
	if (ret < 0)
		return ret;

	ctx->typec.cc_status = ret;

	cc1_rd = ret & RS_PHYTIUM_CC1_RD ? 1 : 0;
	cc2_rd = ret & RS_PHYTIUM_CC2_RD ? 1 : 0;
	cc1_ra = ret & RS_PHYTIUM_CC1_RA ? 1 : 0;
	cc2_ra = ret & RS_PHYTIUM_CC2_RA ? 1 : 0;
	cc1_rp = RS_PHYTIUM_CC1_RP(ret);
	cc2_rp = RS_PHYTIUM_CC2_RP(ret);

	/* Debug cable, nothing to do */
	if (cc1_rd && cc2_rd) {
		ctx->typec.cc_orientation_valid = 0;
		return role_sw_phytium_register_partner(ctx, 0, TYPEC_ACCESSORY_DEBUG);
	}

	if (cc1_ra && cc2_ra) {
		ctx->typec.cc_orientation_valid = 0;
		return role_sw_phytium_register_partner(ctx, 0, TYPEC_ACCESSORY_AUDIO);
	}

	ctx->typec.cc_orientation_valid = 1;

	ret = role_sw_phytium_register_partner(ctx, 1, TYPEC_ACCESSORY_NONE);
	if (ret) {
		dev_err(dev, "register partner\n");
		return ret;
	}

	if (cc1_rd || cc1_rp) {
		typec_set_orientation(ctx->typec.port, TYPEC_ORIENTATION_NORMAL);
		ctx->typec.cc_connect = RS_PHYTIUM_CC1_CONNECTED;
	}

	if (cc2_rd || cc2_rp) {
		typec_set_orientation(ctx->typec.port, TYPEC_ORIENTATION_REVERSE);
		ctx->typec.cc_connect = RS_PHYTIUM_CC2_CONNECTED;
	}

	return 0;
}

static int role_sw_phytium_set_mux(struct role_sw_phytium_data *ctx, int pin_assignment)
{
	int mode = TYPEC_STATE_SAFE;

	switch (pin_assignment) {
	case RS_PHYTIUM_SELECT_PIN_ASSIGMENT_U:
		/* default 4 line USB 3.1 */
		mode = TYPEC_STATE_MODAL;
		break;
	case RS_PHYTIUM_SELECT_PIN_ASSIGMENT_C:
	case RS_PHYTIUM_SELECT_PIN_ASSIGMENT_E:
		/* 4 line DP */
		mode = TYPEC_STATE_SAFE;
		break;
	case RS_PHYTIUM_SELECT_PIN_ASSIGMENT_D:
		/* 2 line DP, 2 line USB */
		mode = TYPEC_MODE_USB3;
		break;
	default:
		mode = TYPEC_STATE_SAFE;
		break;
	}

	ctx->typec.pin_assignment = pin_assignment;

	return typec_set_mode(ctx->typec.port, mode);
}

static int role_sw_phytium_set_usb_role(struct role_sw_phytium_data *ctx, enum usb_role role)
{
	if (!ctx->typec.role_sw)
		return 0;

	return usb_role_switch_set_role(ctx->typec.role_sw, role);
}

static int role_sw_phytium_data_role_detect(struct role_sw_phytium_data *ctx)
{
	int ret;

	ret = role_sw_phytium_reg_read(ctx->spi_client, RS_PHYTIUM_SYSTEM_STSTUS);
	if (ret < 0)
		return ret;

	ctx->typec.data_role = (ret & RS_PHYTIUM_DATA_ROLE) ? TYPEC_HOST : TYPEC_DEVICE;
	ctx->typec.vconn_role = (ret & RS_PHYTIUM_VCONN_STATUS) ? TYPEC_SOURCE : TYPEC_SINK;

	typec_set_data_role(ctx->typec.port, ctx->typec.data_role);

	typec_set_vconn_role(ctx->typec.port, ctx->typec.vconn_role);

	if (ctx->typec.data_role == TYPEC_HOST)
		return role_sw_phytium_set_usb_role(ctx, USB_ROLE_HOST);

	return role_sw_phytium_set_usb_role(ctx, USB_ROLE_DEVICE);
}

static int role_sw_phytium_power_role_detect(struct role_sw_phytium_data *ctx)
{
	int ret;

	ret = role_sw_phytium_reg_read(ctx->spi_client, RS_PHYTIUM_SYSTEM_STSTUS);
	if (ret < 0)
		return ret;

	ctx->typec.power_role = (ret & RS_PHYTIUM_SINK_STATUS) ? TYPEC_SINK : TYPEC_SOURCE;

	if (ctx->typec.power_role == TYPEC_SOURCE) {
		ctx->typec.request_current = RS_PHYTIUM_DEF_1_5A;
		ctx->typec.request_voltage = RS_PHYTIUM_DEF_5V;
	}

	typec_set_pwr_role(ctx->typec.port, ctx->typec.power_role);

	return 0;
}

static int role_sw_phytium_cc_status_detect(struct role_sw_phytium_data *ctx)
{
	role_sw_phytium_detect_cc_orientation(ctx);
	role_sw_phytium_detect_power_mode(ctx);

	return 0;
}

static void role_sw_phytium_partner_unregister_altmode(struct role_sw_phytium_data *ctx)
{
	int i;

	ctx->typec.dp_altmode_enter = 0;
	ctx->typec.cust_altmode_enter = 0;

	for (i = 0; i < RS_PHYTIUM_MAX_ALTMODE; i++)
		if (ctx->typec.amode[i]) {
			typec_unregister_altmode(ctx->typec.amode[i]);
			ctx->typec.amode[i] = NULL;
		}

	ctx->typec.pin_assignment = 0;
}

static int role_sw_phytium_typec_register_altmode(struct role_sw_phytium_data *ctx,
					  int svid, int vdo)
{
	struct device *dev = &ctx->spi_client->dev;
	struct typec_altmode_desc desc;
	int err;
	int i;

	desc.svid = svid;
	desc.vdo = vdo;

	for (i = 0; i < RS_PHYTIUM_MAX_ALTMODE; i++)
		if (!ctx->typec.amode[i])
			break;

	desc.mode = i + 1; /* start with 1 */

	if (i >= RS_PHYTIUM_MAX_ALTMODE) {
		dev_err(dev, "no altmode space for registering\n");
		return -ENOMEM;
	}

	ctx->typec.amode[i] = typec_partner_register_altmode(ctx->typec.partner,
							     &desc);
	if (IS_ERR(ctx->typec.amode[i])) {
		dev_err(dev, "failed to register altmode\n");
		err = PTR_ERR(ctx->typec.amode[i]);
		ctx->typec.amode[i] = NULL;
		return err;
	}

	return 0;
}

static void role_sw_phytium_unregister_partner(struct role_sw_phytium_data *ctx)
{
	if (ctx->typec.partner) {
		typec_unregister_partner(ctx->typec.partner);
		ctx->typec.partner = NULL;
	}
}

static int role_sw_phytium_update_altmode(struct role_sw_phytium_data *ctx, int svid)
{
	int i;

	if (svid == RS_PHYTIUM_DP_SVID)
		ctx->typec.dp_altmode_enter = 1;
	else
		ctx->typec.cust_altmode_enter = 1;

	for (i = 0; i < RS_PHYTIUM_MAX_ALTMODE; i++) {
		if (!ctx->typec.amode[i])
			continue;

		if (ctx->typec.amode[i]->svid == svid) {
			typec_altmode_update_active(ctx->typec.amode[i], true);
			typec_altmode_notify(ctx->typec.amode[i],
					     ctx->typec.pin_assignment,
					     &ctx->typec.data);
			break;
		}
	}

	return 0;
}

static int role_sw_phytium_register_altmode(struct role_sw_phytium_data *ctx,
				    bool dp_altmode, u8 *buf)
{
	int ret;
	int svid;
	int mid;

	if (!ctx->typec.partner)
		return 0;

	svid = RS_PHYTIUM_DP_SVID;
	if (dp_altmode) {
		mid = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

		return role_sw_phytium_typec_register_altmode(ctx, svid, mid);
	}

	svid = (buf[3] << 8) | buf[2];
	if ((buf[0] & RS_PHYTIUM_VDM_CMD_AND_ACK_MASK) !=
			(RS_PHYTIUM_VDM_ACK | RS_PHYTIUM_VDM_CMD_ENTER_MODE))
		return role_sw_phytium_update_altmode(ctx, svid);

	if ((buf[0] & RS_PHYTIUM_VDM_CMD_AND_ACK_MASK) !=
			(RS_PHYTIUM_VDM_ACK | RS_PHYTIUM_VDM_CMD_DIS_MOD))
		return 0;

	mid = buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24);

	ret = role_sw_phytium_typec_register_altmode(ctx, svid, mid);
	if (ctx->typec.cust_altmode_enter)
		ret |= role_sw_phytium_update_altmode(ctx, svid);

	return ret;
}

static int role_sw_phytium_parse_cmd(struct role_sw_phytium_data *ctx, u8 type, u8 *buf, u8 len)
{
	struct device *dev = &ctx->spi_client->dev;
	u8 cur_50ma, vol_100mv;

	switch (type) {
	case RS_PHYTIUM_TYPE_SRC_CAP:
		cur_50ma = role_sw_phytium_reg_read(ctx->spi_client, RS_PHYTIUM_REQUEST_CURRENT);
		vol_100mv = role_sw_phytium_reg_read(ctx->spi_client, RS_PHYTIUM_REQUEST_VOLTAGE);

		ctx->typec.request_voltage = vol_100mv * RS_PHYTIUM_VOLTAGE_UNIT;
		ctx->typec.request_current = cur_50ma * RS_PHYTIUM_CURRENT_UNIT;

		ctx->psy_online = RS_PHYTIUM_PSY_FIXED_ONLINE;
		ctx->usb_type = POWER_SUPPLY_USB_TYPE_PD;
		power_supply_changed(ctx->psy);
		break;
	case RS_PHYTIUM_TYPE_SNK_CAP:
		break;
	case RS_PHYTIUM_TYPE_SVID:
		break;
	case RS_PHYTIUM_TYPE_SNK_IDENTITY:
		break;
	case RS_PHYTIUM_TYPE_GET_DP_ALT_ENTER:
		/* DP alt mode enter success */
		if (buf[0])
			role_sw_phytium_update_altmode(ctx, RS_PHYTIUM_DP_SVID);
		break;
	case RS_PHYTIUM_TYPE_DP_ALT_ENTER:
		/* Update DP altmode */
		role_sw_phytium_update_altmode(ctx, RS_PHYTIUM_DP_SVID);
		break;
	case RS_PHYTIUM_TYPE_OBJ_REQ:
		role_sw_phytium_detect_power_mode(ctx);
		break;
	case RS_PHYTIUM_TYPE_DP_CONFIGURE:
		role_sw_phytium_set_mux(ctx, buf[1]);
		break;
	case RS_PHYTIUM_TYPE_DP_DISCOVER_MODES_INFO:
		/* Make sure discover modes valid */
		if (buf[0] | buf[1])
			/* Register DP Altmode */
			role_sw_phytium_register_altmode(ctx, 1, buf);
		break;
	case RS_PHYTIUM_TYPE_VDM:
		/* Register other altmode */
		role_sw_phytium_register_altmode(ctx, 0, buf);
		break;
	default:
		dev_err(dev, "ignore message(0x%.02x).\n", type);
		break;
	}

	return 0;
}

static u8 checksum(struct device *dev, u8 *buf, u8 len)
{
	u8 ret = 0;
	u8 i;

	for (i = 0; i < len; i++)
		ret += buf[i];

	return ret;
}

static int role_sw_phytium_read_msg_ctrl_status(struct i2c_client *client)
{
	return role_sw_phytium_reg_read(client, RS_PHYTIUM_CMD_SEND_BUF);
}

static int role_sw_phytium_wait_msg_empty(struct i2c_client *client)
{
	int val;

	return readx_poll_timeout(role_sw_phytium_read_msg_ctrl_status,
				  client, val, (val < 0) || (val == 0),
				  2000, 2000 * 150);
}

static int role_sw_phytium_send_msg(struct role_sw_phytium_data *ctx, u8 type, u8 *buf, u8 size)
{
	struct device *dev = &ctx->spi_client->dev;
	struct role_sw_phytium_fw_msg *msg = &ctx->send_msg;
	u8 crc;
	int ret;

	size = min_t(u8, size, (u8)RS_PHYTIUM_MAX_BUF_LEN);
	memcpy(msg->buf, buf, size);
	msg->msg_type = type;
	/* msg len equals buffer length + msg_type */
	msg->msg_len = size + 1;

	/* Do CRC check for all buffer data and msg_len and msg_type */
	crc = checksum(dev, (u8 *)msg, size + RS_PHYTIUM_HEADER_LEN);
	msg->buf[size] = 0 - crc;

	ret = role_sw_phytium_wait_msg_empty(ctx->spi_client);
	if (ret)
		return ret;

	ret = role_sw_phytium_reg_block_write(ctx->spi_client,
				      RS_PHYTIUM_CMD_SEND_BUF + 1, size + RS_PHYTIUM_HEADER_LEN,
				      &msg->msg_type);
	ret |= role_sw_phytium_reg_write(ctx->spi_client, RS_PHYTIUM_CMD_SEND_BUF,
				 msg->msg_len);
	return ret;
}

static int role_sw_phytium_process_cmd(struct role_sw_phytium_data *ctx)
{
	struct device *dev = &ctx->spi_client->dev;
	struct role_sw_phytium_fw_msg *msg = &ctx->recv_msg;
	u8 len;
	u8 crc;
	int ret;

	/* Read message from firmware */
	ret = role_sw_phytium_reg_block_read(ctx->spi_client, RS_PHYTIUM_CMD_RECV_BUF,
				     RS_PHYTIUM_MSG_LEN, (u8 *)msg);
	if (ret < 0)
		return 0;

	if (!msg->msg_len)
		return 0;

	ret = role_sw_phytium_reg_write(ctx->spi_client, RS_PHYTIUM_CMD_RECV_BUF, 0);
	if (ret)
		return ret;

	len = msg->msg_len & RS_PHYTIUM_MSG_LEN_MASK;
	crc = checksum(dev, (u8 *)msg, len + RS_PHYTIUM_HEADER_LEN);
	if (crc) {
		dev_err(dev, "message error crc(0x%.02x)\n", crc);
		return -ERANGE;
	}

	return role_sw_phytium_parse_cmd(ctx, msg->msg_type, msg->buf, len - 1);
}

static void role_sw_phytium_translate_payload(struct device *dev, __le32 *payload,
				      u32 *pdo, int nr, const char *type)
{
	int i;

	if (nr > PDO_MAX_OBJECTS) {
		dev_err(dev, "nr(%d) exceed PDO_MAX_OBJECTS(%d)\n",
			nr, PDO_MAX_OBJECTS);

		return;
	}

	for (i = 0; i < nr; i++)
		payload[i] = cpu_to_le32(pdo[i]);
}

static int role_sw_phytium_config(struct role_sw_phytium_data *ctx)
{
	struct device *dev = &ctx->spi_client->dev;
	struct role_sw_phytium_typec_params *typecp = &ctx->typec;
	__le32 payload[PDO_MAX_OBJECTS];
	int ret;

	/* Config PD FW work under PD 2.0 */
	ret = role_sw_phytium_reg_write(ctx->spi_client, RS_PHYTIUM_PD_REV_INIT, PD_REV20);
	ret |= role_sw_phytium_reg_write(ctx->tcpc_client, RS_PHYTIUM_FW_CTRL_0,
				 RS_PHYTIUM_UNSTRUCT_VDM_EN | RS_PHYTIUM_DELAY_200MS |
				 RS_PHYTIUM_VSAFE1 | RS_PHYTIUM_FRS_EN);
	ret |= role_sw_phytium_reg_write(ctx->spi_client, RS_PHYTIUM_FW_CTRL_1,
				 RS_PHYTIUM_AUTO_PD_EN | RS_PHYTIUM_FORCE_SEND_RDO);

	/* Set VBUS current threshold */
	ret |= role_sw_phytium_reg_write(ctx->tcpc_client, RS_PHYTIUM_VBUS_THRESHOLD_H, 0xff);
	ret |= role_sw_phytium_reg_write(ctx->tcpc_client, RS_PHYTIUM_VBUS_THRESHOLD_L, 0x03);

	/* Fix dongle compatible issue */
	ret |= role_sw_phytium_reg_write(ctx->tcpc_client, RS_PHYTIUM_FW_PARAM,
				 role_sw_phytium_reg_read(ctx->tcpc_client, RS_PHYTIUM_FW_PARAM) |
				 RS_PHYTIUM_DONGLE_IOP);
	ret |= role_sw_phytium_reg_write(ctx->spi_client, RS_PHYTIUM_INT_MASK, 0);

	ret |= role_sw_phytium_reg_write(ctx->spi_client, RS_PHYTIUM_PD_EXT_MSG_CTRL, 0xFF);
	if (ret)
		return ret;

	if (typecp->caps_flags & RS_PHYTIUM_HAS_SOURCE_CAP) {
		role_sw_phytium_translate_payload(dev, payload, typecp->src_pdo,
					  typecp->src_pdo_nr, "source");
		role_sw_phytium_send_msg(ctx, RS_PHYTIUM_TYPE_SRC_CAP, (u8 *)&payload,
				 typecp->src_pdo_nr * 4);
		role_sw_phytium_send_msg(ctx, RS_PHYTIUM_TYPE_SNK_IDENTITY, snk_identity,
				 sizeof(snk_identity));
		role_sw_phytium_send_msg(ctx, RS_PHYTIUM_TYPE_SET_SNK_DP_CAP, dp_caps,
				 sizeof(dp_caps));
	}

	if (typecp->caps_flags & RS_PHYTIUM_HAS_SINK_CAP) {
		role_sw_phytium_translate_payload(dev, payload, typecp->sink_pdo,
					  typecp->sink_pdo_nr, "sink");
		role_sw_phytium_send_msg(ctx, RS_PHYTIUM_TYPE_SNK_CAP, (u8 *)&payload,
				 typecp->sink_pdo_nr * 4);
	}

	if (typecp->caps_flags & RS_PHYTIUM_HAS_SINK_WATT) {
		if (typecp->sink_watt) {
			ret |= role_sw_phytium_reg_write(ctx->spi_client, RS_PHYTIUM_MAX_POWER,
						 typecp->sink_watt);
			/* Set min power to 1W */
			ret |= role_sw_phytium_reg_write(ctx->spi_client, RS_PHYTIUM_MIN_POWER, 2);
		}

		if (typecp->sink_voltage)
			ret |= role_sw_phytium_reg_write(ctx->spi_client, RS_PHYTIUM_MAX_VOLTAGE,
					  typecp->sink_voltage);
		if (ret)
			return ret;
	}

	if (!typecp->caps_flags)
		usleep_range(5000, 6000);

	ctx->fw_version = role_sw_phytium_reg_read(ctx->spi_client, RS_PHYTIUM_FW_VER);
	ctx->fw_subversion = role_sw_phytium_reg_read(ctx->spi_client, RS_PHYTIUM_FW_SUBVER);

	return 0;
}

static void role_sw_phytium_chip_standby(struct role_sw_phytium_data *ctx)
{
	int ret;
	u8 cc1, cc2;
	struct device *dev = &ctx->spi_client->dev;

	ret = role_sw_phytium_reg_write(ctx->spi_client, RS_PHYTIUM_OCM_CTRL_0,
				role_sw_phytium_reg_read(ctx->spi_client, RS_PHYTIUM_OCM_CTRL_0) |
				RS_PHYTIUM_OCM_RESET);
	ret |= role_sw_phytium_reg_write(ctx->tcpc_client, RS_PHYTIUM_ANALOG_CTRL_10, 0x80);
	/* Set TCPC to RD and DRP enable */
	cc1 = TCPC_ROLE_CTRL_CC_RD << TCPC_ROLE_CTRL_CC1_SHIFT;
	cc2 = TCPC_ROLE_CTRL_CC_RD << TCPC_ROLE_CTRL_CC2_SHIFT;
	ret |= role_sw_phytium_reg_write(ctx->tcpc_client, TCPC_ROLE_CTRL,
				 TCPC_ROLE_CTRL_DRP | cc1 | cc2);

	/* Send DRP toggle command */
	ret |= role_sw_phytium_reg_write(ctx->tcpc_client, TCPC_COMMAND,
				 TCPC_CMD_LOOK4CONNECTION);

	/* Send TCPC enter standby command */
	ret |= role_sw_phytium_reg_write(ctx->tcpc_client,
				 TCPC_COMMAND, TCPC_CMD_I2C_IDLE);
	if (ret)
		dev_err(dev, "Chip standby failed\n");
}

static void role_sw_phytium_work_func(struct work_struct *work)
{
	int ret;
	u8 buf[RS_PHYTIUM_STATUS_LEN];
	u8 int_change; /* Interrupt change */
	u8 int_status; /* Firmware status update */
	u8 alert0, alert1; /* Interrupt alert source */
	struct role_sw_phytium_data *ctx = container_of(work, struct role_sw_phytium_data, work);
	struct device *dev = &ctx->spi_client->dev;

	mutex_lock(&ctx->lock);

	/* Read interrupt change status */
	ret = role_sw_phytium_reg_block_read(ctx->spi_client, RS_PHYTIUM_INT_STS,
						 RS_PHYTIUM_STATUS_LEN, buf);
	if (ret < 0) {
		/* Power standby mode, just return */
		goto unlock;
	}
	int_change = buf[0];
	int_status = buf[1];

	/* Read alert register */
	ret = role_sw_phytium_reg_block_read(ctx->tcpc_client, RS_PHYTIUM_ALERT_0,
						 RS_PHYTIUM_STATUS_LEN, buf);
	if (ret < 0)
		goto unlock;

	alert0 = buf[0];
	alert1 = buf[1];

	/* Clear interrupt and alert status */
	ret = role_sw_phytium_reg_write(ctx->spi_client, RS_PHYTIUM_INT_STS, 0);
	ret |= role_sw_phytium_reg_write(ctx->tcpc_client, RS_PHYTIUM_ALERT_0, alert0);
	ret |= role_sw_phytium_reg_write(ctx->tcpc_client, RS_PHYTIUM_ALERT_1, alert1);
	if (ret)
		goto unlock;

	if (alert1 & RS_PHYTIUM_INTP_POW_OFF) {
		role_sw_phytium_partner_unregister_altmode(ctx);
		if (role_sw_phytium_set_usb_role(ctx, USB_ROLE_NONE))
			dev_err(dev, "Set usb role\n");
		role_sw_phytium_unregister_partner(ctx);
		ctx->psy_online = RS_PHYTIUM_PSY_OFFLINE;
		ctx->usb_type = POWER_SUPPLY_USB_TYPE_C;
		ctx->typec.request_voltage = 0;
		ctx->typec.request_current = 0;
		power_supply_changed(ctx->psy);
		role_sw_phytium_chip_standby(ctx);
		goto unlock;
	}

	if ((alert0 & RS_PHYTIUM_SOFTWARE_INT) && (int_change & RS_PHYTIUM_OCM_BOOT_UP)) {
		if (role_sw_phytium_config(ctx))
			dev_err(dev, "Config failed\n");
		if (role_sw_phytium_data_role_detect(ctx))
			dev_err(dev, "set PD data role\n");
		if (role_sw_phytium_power_role_detect(ctx))
			dev_err(dev, "set PD power role\n");
		role_sw_phytium_set_mux(ctx, RS_PHYTIUM_SELECT_PIN_ASSIGMENT_C);
	}

	if (alert0 & RS_PHYTIUM_RECEIVED_MSG)
		role_sw_phytium_process_cmd(ctx);

	ret = (int_status & RS_PHYTIUM_DATA_ROLE) ? TYPEC_HOST : TYPEC_DEVICE;
	if (ctx->typec.data_role != ret)
		if (role_sw_phytium_data_role_detect(ctx))
			dev_err(dev, "set PD data role\n");

	ret = (int_status & RS_PHYTIUM_SINK_STATUS) ? TYPEC_SINK : TYPEC_SOURCE;
	if (ctx->typec.power_role != ret)
		if (role_sw_phytium_power_role_detect(ctx))
			dev_err(dev, "set PD power role\n");

	if ((alert0 & RS_PHYTIUM_SOFTWARE_INT) && (int_change & RS_PHYTIUM_CC_STATUS_CHANGE))
		role_sw_phytium_cc_status_detect(ctx);

unlock:
	mutex_unlock(&ctx->lock);
}

static irqreturn_t role_sw_phytium_intr_isr(int irq, void *data)
{
	struct role_sw_phytium_data *ctx = (struct role_sw_phytium_data *)data;

	queue_work(ctx->workqueue, &ctx->work);

	return IRQ_HANDLED;
}

static int role_sw_phytium_register_i2c_dummy_clients(struct role_sw_phytium_data *ctx,
					      struct i2c_client *client)
{
	int i;
	u8 spi_addr;

	for (i = 0; i < ARRAY_SIZE(role_sw_phytium_i2c_addr); i++) {
		if (client->addr == (role_sw_phytium_i2c_addr[i].tcpc_address >> 1)) {
			spi_addr = role_sw_phytium_i2c_addr[i].spi_address >> 1;
			ctx->spi_client = i2c_new_dummy_device(client->adapter,
							       spi_addr);
			if (!IS_ERR(ctx->spi_client))
				return 0;
		}
	}

	dev_err(&client->dev, "unable to get SPI slave\n");
	return -ENOMEM;
}

static void role_sw_phytium_port_unregister_altmodes(struct typec_altmode **adev)
{
	int i;

	for (i = 0; i < RS_PHYTIUM_MAX_ALTMODE; i++)
		if (adev[i]) {
			typec_unregister_altmode(adev[i]);
			adev[i] = NULL;
		}
}

static int role_sw_phytium_usb_mux_set(struct typec_mux_dev *mux,
			       struct typec_mux_state *state)
{
	struct role_sw_phytium_data *ctx = typec_mux_get_drvdata(mux);
	struct device *dev = &ctx->spi_client->dev;
	int has_dp;

	has_dp = (state->alt && state->alt->svid == USB_TYPEC_DP_SID &&
		  state->alt->mode == USB_TYPEC_DP_MODE);
	if (!has_dp)
		dev_err(dev, "dp altmode not register\n");

	return 0;
}

static int role_sw_phytium_usb_set_orientation(struct typec_switch_dev *sw,
				       enum typec_orientation orientation)
{
	/* No need set */

	return 0;
}

static int role_sw_phytium_register_switch(struct role_sw_phytium_data *ctx,
				   struct device *dev,
				   struct fwnode_handle *fwnode)
{
	struct typec_switch_desc sw_desc = { };

	sw_desc.fwnode = fwnode;
	sw_desc.drvdata = ctx;
	sw_desc.name = fwnode_get_name(fwnode);
	sw_desc.set = role_sw_phytium_usb_set_orientation;

	ctx->typec.typec_switch = typec_switch_register(dev, &sw_desc);
	if (IS_ERR(ctx->typec.typec_switch)) {
		dev_err(dev, "switch register failed\n");
		return PTR_ERR(ctx->typec.typec_switch);
	}

	return 0;
}

static int role_sw_phytium_register_mux(struct role_sw_phytium_data *ctx,
				struct device *dev,
				struct fwnode_handle *fwnode)
{
	struct typec_mux_desc mux_desc = { };

	mux_desc.fwnode = fwnode;
	mux_desc.drvdata = ctx;
	mux_desc.name = fwnode_get_name(fwnode);
	mux_desc.set = role_sw_phytium_usb_mux_set;

	ctx->typec.typec_mux = typec_mux_register(dev, &mux_desc);
	if (IS_ERR(ctx->typec.typec_mux)) {
		dev_err(dev, "mux register failed\n");
		return PTR_ERR(ctx->typec.typec_mux);
	}

	return 0;
}

static void role_sw_phytium_unregister_mux(struct role_sw_phytium_data *ctx)
{
	if (ctx->typec.typec_mux) {
		typec_mux_unregister(ctx->typec.typec_mux);
		ctx->typec.typec_mux = NULL;
	}
}

static void role_sw_phytium_unregister_switch(struct role_sw_phytium_data *ctx)
{
	if (ctx->typec.typec_switch) {
		typec_switch_unregister(ctx->typec.typec_switch);
		ctx->typec.typec_switch = NULL;
	}
}

static int role_sw_phytium_typec_switch_probe(struct role_sw_phytium_data *ctx,
				      struct device *dev)
{
	int ret;
	struct fwnode_handle *fwnode;
	char sw_str[32] = {0}, almode_str[32] = {0};

	if (has_acpi_companion(dev)) {
		strscpy(sw_str, "ORSW", sizeof(sw_str));
		strscpy(almode_str, "ALSW", sizeof(almode_str));
	} else {
		strscpy(sw_str, "orientation_switch", sizeof(sw_str));
		strscpy(almode_str, "mode_switch", sizeof(almode_str));
	}

	fwnode = device_get_named_child_node(dev, sw_str);
	if (!fwnode) {
		dev_err(dev, "Err:cannot find %s\n", sw_str);
		return -EINVAL;
	}

	ret = role_sw_phytium_register_switch(ctx, dev, fwnode);
	if (ret) {
		dev_err(dev, "failed register switch");
		return ret;
	}

	fwnode = device_get_named_child_node(dev, almode_str);
	if (!fwnode) {
		dev_err(dev, "no typec mux exist");
		ret = -ENODEV;
		goto unregister_switch;
	}
	ret = role_sw_phytium_register_mux(ctx, dev, fwnode);
	if (ret) {
		dev_err(dev, "failed register mode switch");
		ret = -ENODEV;
		goto unregister_switch;
	}

	return 0;

unregister_switch:
	role_sw_phytium_unregister_switch(ctx);

	return ret;
}

static int role_sw_phytium_typec_port_probe(struct role_sw_phytium_data *ctx,
				    struct device *dev)
{
	struct typec_capability *cap = &ctx->typec.caps;
	struct role_sw_phytium_typec_params *typecp = &ctx->typec;
	struct fwnode_handle *fwnode;
	const char *buf;
	int ret, i;
	char conc_str[32] = {0};

	if (has_acpi_companion(dev))
		strscpy(conc_str, "CONC", sizeof(conc_str));
	else
		strscpy(conc_str, "connector", sizeof(conc_str));

	fwnode = device_get_named_child_node(dev, conc_str);
	if (!fwnode)
		return -EINVAL;

	ret = fwnode_property_read_string(fwnode, "power-role", &buf);
	if (ret) {
		dev_err(dev, "power-role not found: %d\n", ret);
		return ret;
	}

	ret = typec_find_port_power_role(buf);
	if (ret < 0)
		return ret;
	cap->type = ret;

	ret = fwnode_property_read_string(fwnode, "data-role", &buf);
	if (ret) {
		dev_err(dev, "data-role not found: %d\n", ret);
		return ret;
	}

	ret = typec_find_port_data_role(buf);
	if (ret < 0)
		return ret;
	cap->data = ret;

	ret = fwnode_property_read_string(fwnode, "try-power-role", &buf);
	if (ret) {
		dev_err(dev, "try-power-role not found: %d\n", ret);
		return ret;
	}

	ret = typec_find_power_role(buf);
	if (ret < 0)
		return ret;
	cap->prefer_role = ret;

	/* Get source pdos */
	ret = fwnode_property_count_u32(fwnode, "source-pdos");
	if (ret > 0) {
		typecp->src_pdo_nr = min_t(u8, ret, PDO_MAX_OBJECTS);
		ret = fwnode_property_read_u32_array(fwnode, "source-pdos",
						     typecp->src_pdo,
						     typecp->src_pdo_nr);
		if (ret < 0) {
			dev_err(dev, "source cap validate failed: %d\n", ret);
			return -EINVAL;
		}

		typecp->caps_flags |= RS_PHYTIUM_HAS_SOURCE_CAP;
	}

	ret = fwnode_property_count_u32(fwnode, "sink-pdos");
	if (ret > 0) {
		typecp->sink_pdo_nr = min_t(u8, ret, PDO_MAX_OBJECTS);
		ret = fwnode_property_read_u32_array(fwnode, "sink-pdos",
						     typecp->sink_pdo,
						     typecp->sink_pdo_nr);
		if (ret < 0) {
			dev_err(dev, "sink cap validate failed: %d\n", ret);
			return -EINVAL;
		}

		for (i = 0; i < typecp->sink_pdo_nr; i++) {
			ret = 0;
			switch (pdo_type(typecp->sink_pdo[i])) {
			case PDO_TYPE_FIXED:
				ret = pdo_fixed_voltage(typecp->sink_pdo[i]);
				break;
			case PDO_TYPE_BATT:
			case PDO_TYPE_VAR:
				ret = pdo_max_voltage(typecp->sink_pdo[i]);
				break;
			case PDO_TYPE_APDO:
			default:
				ret = 0;
				break;
			}

			/* 100mv per unit */
			typecp->sink_voltage = max(5000, ret) / 100;
		}

		typecp->caps_flags |= RS_PHYTIUM_HAS_SINK_CAP;
	}

	if (!fwnode_property_read_u32(fwnode, "op-sink-microwatt", &ret)) {
		typecp->sink_watt = ret / 500000; /* 500mw per unit */
		typecp->caps_flags |= RS_PHYTIUM_HAS_SINK_WATT;
	}

	cap->fwnode = fwnode;

	ctx->typec.role_sw = usb_role_switch_get(dev);
	if (IS_ERR(ctx->typec.role_sw)) {
		dev_err(dev, "USB role switch not found.\n");
		ctx->typec.role_sw = NULL;
	}

	ctx->typec.port = typec_register_port(dev, cap);
	if (IS_ERR(ctx->typec.port)) {
		ret = PTR_ERR(ctx->typec.port);
		ctx->typec.port = NULL;
		dev_err(dev, "Failed to register type c port %d\n", ret);
		return ret;
	}

	typec_port_register_altmodes(ctx->typec.port, NULL, ctx,
				     ctx->typec.port_amode,
				     RS_PHYTIUM_MAX_ALTMODE);
	return 0;
}

static int role_sw_phytium_typec_check_connection(struct role_sw_phytium_data *ctx)
{
	int ret;

	ret = role_sw_phytium_reg_read(ctx->spi_client, RS_PHYTIUM_FW_VER);
	if (ret < 0)
		return 0; /* No device attached in typec port */

	/* Clear interrupt and alert status */
	ret = role_sw_phytium_reg_write(ctx->spi_client, RS_PHYTIUM_INT_STS, 0);
	ret |= role_sw_phytium_reg_write(ctx->tcpc_client, RS_PHYTIUM_ALERT_0, 0xFF);
	ret |= role_sw_phytium_reg_write(ctx->tcpc_client, RS_PHYTIUM_ALERT_1, 0xFF);
	if (ret)
		return ret;

	ret = role_sw_phytium_cc_status_detect(ctx);
	ret |= role_sw_phytium_power_role_detect(ctx);
	ret |= role_sw_phytium_data_role_detect(ctx);
	ret |= role_sw_phytium_set_mux(ctx, RS_PHYTIUM_SELECT_PIN_ASSIGMENT_C);
	if (ret)
		return ret;

	ret = role_sw_phytium_send_msg(ctx, RS_PHYTIUM_TYPE_GET_DP_ALT_ENTER, NULL, 0);
	ret |= role_sw_phytium_send_msg(ctx, RS_PHYTIUM_TYPE_GET_DP_DISCOVER_MODES_INFO, NULL, 0);

	return ret;
}

static int __maybe_unused role_sw_phytium_runtime_pm_suspend(struct device *dev)
{
	struct role_sw_phytium_data *ctx = dev_get_drvdata(dev);

	mutex_lock(&ctx->lock);

	role_sw_phytium_partner_unregister_altmode(ctx);

	if (ctx->typec.partner)
		role_sw_phytium_unregister_partner(ctx);

	mutex_unlock(&ctx->lock);

	return 0;
}

static int __maybe_unused role_sw_phytium_runtime_pm_resume(struct device *dev)
{
	struct role_sw_phytium_data *ctx = dev_get_drvdata(dev);

	mutex_lock(&ctx->lock);
	/* Detect PD connection */
	if (role_sw_phytium_typec_check_connection(ctx))
		dev_err(dev, "check connection");

	mutex_unlock(&ctx->lock);

	return 0;
}

static const struct dev_pm_ops role_sw_phytium_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(role_sw_phytium_runtime_pm_suspend,
			   role_sw_phytium_runtime_pm_resume, NULL)
};

static void role_sw_phytium_get_gpio_irq_acpi(struct role_sw_phytium_data *ctx)
{
	struct device *dev = &ctx->tcpc_client->dev;

	if (!ctx->adev) {
		dev_err(dev, "Err: no valid fwnode\n");
		return;
	}

	ctx->intp_irq = acpi_dev_gpio_irq_get(ctx->adev, 0);
	if (ctx->intp_irq < 0)
		dev_err(dev, "failed to get GPIO IRQ\n");
}

static void role_sw_phytium_get_gpio_irq(struct role_sw_phytium_data *ctx)
{
	struct device *dev = &ctx->tcpc_client->dev;

	ctx->intp_gpiod = devm_gpiod_get_optional(dev, "interrupt", GPIOD_IN);
	if (IS_ERR_OR_NULL(ctx->intp_gpiod)) {
		dev_err(dev, "no interrupt gpio property\n");
		return;
	}

	ctx->intp_irq = gpiod_to_irq(ctx->intp_gpiod);
	if (ctx->intp_irq < 0)
		dev_err(dev, "failed to get GPIO IRQ\n");
}

static enum power_supply_usb_type role_sw_phytium_psy_usb_types[] = {
	POWER_SUPPLY_USB_TYPE_C,
	POWER_SUPPLY_USB_TYPE_PD,
	POWER_SUPPLY_USB_TYPE_PD_PPS,
};

static enum power_supply_property role_sw_phytium_psy_props[] = {
	POWER_SUPPLY_PROP_USB_TYPE,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_MIN,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

static int role_sw_phytium_psy_set_prop(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct role_sw_phytium_data *ctx = power_supply_get_drvdata(psy);
	int ret = 0;

	if (psp == POWER_SUPPLY_PROP_ONLINE)
		ctx->psy_online = val->intval;
	else
		ret = -EINVAL;

	power_supply_changed(ctx->psy);
	return ret;
}

static int role_sw_phytium_psy_prop_writeable(struct power_supply *psy,
				      enum power_supply_property psp)
{
	return psp == POWER_SUPPLY_PROP_ONLINE;
}

static int role_sw_phytium_psy_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct role_sw_phytium_data *ctx = power_supply_get_drvdata(psy);
	int ret = 0;

	if (!ctx) {
		pr_err("ctx is null.\n");
		return -EINVAL;
	}
	switch (psp) {
	case POWER_SUPPLY_PROP_USB_TYPE:
		val->intval = ctx->usb_type;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = ctx->psy_online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = (ctx->psy_online) ?
			ctx->typec.request_voltage * 1000 : 0;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = (ctx->psy_online) ?
			ctx->typec.request_current * 1000 : 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int role_sw_phytium_psy_register(struct role_sw_phytium_data *ctx)
{
	struct power_supply_desc *psy_desc = &ctx->psy_desc;
	struct power_supply_config psy_cfg = {};
	char *psy_name;

	psy_name = devm_kasprintf(ctx->dev, GFP_KERNEL, "anx7411-source-psy-%s",
				  dev_name(ctx->dev));
	if (!psy_name)
		return -ENOMEM;

	psy_desc->name = psy_name;
	psy_desc->type = POWER_SUPPLY_TYPE_USB;
	psy_desc->usb_types = role_sw_phytium_psy_usb_types;
	psy_desc->num_usb_types = ARRAY_SIZE(role_sw_phytium_psy_usb_types);
	psy_desc->properties = role_sw_phytium_psy_props;
	psy_desc->num_properties = ARRAY_SIZE(role_sw_phytium_psy_props);

	psy_desc->get_property = role_sw_phytium_psy_get_prop;
	psy_desc->set_property = role_sw_phytium_psy_set_prop;
	psy_desc->property_is_writeable = role_sw_phytium_psy_prop_writeable;
	psy_cfg.drv_data = ctx;
	ctx->usb_type = POWER_SUPPLY_USB_TYPE_C;
	ctx->psy = devm_power_supply_register(ctx->dev, psy_desc, &psy_cfg);

	if (IS_ERR(ctx->psy))
		dev_warn(ctx->dev, "unable to register psy\n");

	return PTR_ERR_OR_ZERO(ctx->psy);
}

static int role_sw_phytium_i2c_probe(struct i2c_client *client)
{
	struct role_sw_phytium_data *plat;
	struct device *dev = &client->dev;
	int ret;

	dev_info(dev, "probe start\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK))
		return -ENODEV;

	plat = devm_kzalloc(dev, sizeof(*plat), GFP_KERNEL);
	if (!plat)
		return -ENOMEM;

	plat->tcpc_client = client;
	i2c_set_clientdata(client, plat);

	mutex_init(&plat->lock);

	ret = role_sw_phytium_register_i2c_dummy_clients(plat, client);
	if (ret) {
		dev_err(dev, "fail to reserve I2C bus\n");
		return ret;
	}

	ret = role_sw_phytium_typec_switch_probe(plat, dev);
	if (ret) {
		dev_err(dev, "fail to probe typec switch\n");
		goto free_i2c_dummy;
	}

	ret = role_sw_phytium_typec_port_probe(plat, dev);
	if (ret) {
		dev_err(dev, "fail to probe typec property.\n");
		ret = -ENODEV;
		goto free_typec_switch;
	}

	plat->intp_irq = client->irq;
	plat->adev = ACPI_COMPANION(dev);
	if (!client->irq) {
		if (has_acpi_companion(dev))
			role_sw_phytium_get_gpio_irq_acpi(plat);
		else
			role_sw_phytium_get_gpio_irq(plat);
	}

	if (!plat->intp_irq) {
		dev_err(dev, "fail to get interrupt IRQ\n");
		ret = -EINVAL;
		goto free_typec_port;
	}

	plat->dev = dev;
	plat->psy_online = RS_PHYTIUM_PSY_OFFLINE;
	ret = role_sw_phytium_psy_register(plat);
	if (ret) {
		dev_err(dev, "register psy\n");
		goto free_typec_port;
	}

	INIT_WORK(&plat->work, role_sw_phytium_work_func);
	plat->workqueue = alloc_workqueue("rs_phytium_work",
					  WQ_FREEZABLE |
					  WQ_MEM_RECLAIM,
					  1);
	if (!plat->workqueue) {
		dev_err(dev, "fail to create work queue\n");
		ret = -ENOMEM;
		goto free_typec_port;
	}

	ret = devm_request_threaded_irq(dev, plat->intp_irq,
					NULL, role_sw_phytium_intr_isr,
					IRQF_TRIGGER_FALLING |
					IRQF_ONESHOT,
					"rs-phytium-intp", plat);
	if (ret) {
		dev_err(dev, "fail to request irq\n");
		goto free_wq;
	}

	if (role_sw_phytium_typec_check_connection(plat))
		dev_err(dev, "check status\n");

	pm_runtime_enable(dev);

	dev_info(dev, "probe ok\n");
	return 0;

free_wq:
	destroy_workqueue(plat->workqueue);

free_typec_port:
	typec_unregister_port(plat->typec.port);
	role_sw_phytium_port_unregister_altmodes(plat->typec.port_amode);

free_typec_switch:
	role_sw_phytium_unregister_switch(plat);
	role_sw_phytium_unregister_mux(plat);

free_i2c_dummy:
	i2c_unregister_device(plat->spi_client);

	return ret;
}

static void role_sw_phytium_i2c_remove(struct i2c_client *client)
{
	struct role_sw_phytium_data *plat = i2c_get_clientdata(client);

	role_sw_phytium_partner_unregister_altmode(plat);
	role_sw_phytium_unregister_partner(plat);

	if (plat->workqueue)
		destroy_workqueue(plat->workqueue);

	if (plat->spi_client)
		i2c_unregister_device(plat->spi_client);

	if (plat->typec.role_sw)
		usb_role_switch_put(plat->typec.role_sw);

	role_sw_phytium_unregister_mux(plat);

	role_sw_phytium_unregister_switch(plat);

	if (plat->typec.port)
		typec_unregister_port(plat->typec.port);

	role_sw_phytium_port_unregister_altmodes(plat->typec.port_amode);
}

static const struct i2c_device_id role_sw_phytium_id[] = {
	{"role-sw", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, role_sw_phytium_id);

static const struct of_device_id role_sw_phytium_match_table[] = {
	{.compatible = "phytium,role-sw",},
	{},
};
static const struct acpi_device_id role_sw_phytium_acpi_match[] = {
	{ "PHYT8011", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, role_sw_phytium_acpi_match);
static struct i2c_driver role_sw_phytium_driver = {
	.driver = {
		.name = "phytium_role_sw",
		.of_match_table = role_sw_phytium_match_table,
		.acpi_match_table = ACPI_PTR(role_sw_phytium_acpi_match),
		.pm = &role_sw_phytium_pm_ops,
	},
	.probe = role_sw_phytium_i2c_probe,
	.remove = role_sw_phytium_i2c_remove,

	.id_table = role_sw_phytium_id,
};

module_i2c_driver(role_sw_phytium_driver);

MODULE_DESCRIPTION("Phytium USB Type-C PD driver");
MODULE_AUTHOR("Wu Jinyong <wujinyong1788@phytium.com.cn>");
MODULE_LICENSE("GPL");
MODULE_VERSION(RS_PHYTIUM_DRV_VERSION);
