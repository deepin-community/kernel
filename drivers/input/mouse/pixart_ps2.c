#include <linux/delay.h>
#include <linux/device.h>
#include <linux/libps2.h>
#include <linux/input/mt.h>
#include <linux/serio.h>
#include <linux/slab.h>
#include "pixart_ps2.h"

#define MODULE_LOGICAL_MAX_X	1023
#define MODULE_LOGICAL_MAX_Y	579
#define MAX_FINGERS		4
#define SLOTS_NUMBER		MAX_FINGERS

#define PIXART_CMD_REPORT_FORMAT	0x01D8
#define PIXART_CMD_SWITCH_PROTOCOL	0x00DE

enum PIXART_MODE {
	PIXART_MODE_ERROR = (-1),
	PIXART_MODE_REL = 0,
	PIXART_MODE_ABS,
};
enum PIXART_TYPE {
	PIXART_TYPE_ERROR = (-1),
	PIXART_TYPE_CLICKPAD = 0,
	PIXART_TYPE_NORMALPAD,
};

struct pixart_data {
	enum PIXART_MODE mode;
	enum PIXART_TYPE type;
	unsigned int x_max, y_max;
};


static int pixart_mode_detect(struct psmouse *psmouse)
{
	unsigned char param[1];

	if (ps2_command(&psmouse->ps2dev, param, PIXART_CMD_REPORT_FORMAT))
		return -1;

	switch (param[0]) {
	default:
	case (0x00):
		return (PIXART_MODE_REL);
	case (0x01):
		return (PIXART_MODE_ABS);
	}
}

static int pixart_type_detect(struct psmouse *psmouse)
{
	struct ps2dev *ps2dev = &psmouse->ps2dev;
	unsigned char param[3];

	param[0] = 0x0A;
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRATE);
	param[0] = 00;
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRES);
	param[0] = 00;
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRES);
	param[0] = 00;
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRES);
	param[0] = 0x03;
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRES);
	ps2_command(ps2dev, param, PSMOUSE_CMD_GETINFO);

	switch (param[0]) {
	default:
	case (0x0c):
		return (PIXART_TYPE_CLICKPAD);
	case (0x0e):
		return (PIXART_TYPE_NORMALPAD);
	}
}
static int pixart_intellimouse_detect(struct psmouse *psmouse)
{
	struct ps2dev *ps2dev = &psmouse->ps2dev;
	unsigned char param[2];

	param[0] = 200;
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRATE);
	param[0] = 100;
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRATE);
	param[0] =  80;
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRATE);
	ps2_command(ps2dev, param, PSMOUSE_CMD_GETID);

	if (param[0] != 3) {
		return (-1);
	}
	return (0);
}


static void pixart_reset(struct psmouse *psmouse)
{
	ps2_command(&psmouse->ps2dev, NULL, PSMOUSE_CMD_RESET_DIS);
	msleep(100);
	psmouse_reset(psmouse);
}


static void pixart_abs_report(struct psmouse *psmouse, unsigned char report_num)
{
	struct pixart_data *priv = psmouse->private;
	unsigned char *packet = psmouse->packet;
	unsigned int abs_x, abs_y;
	bool tip;
	int id, fingers = 0;
	int i;

	for (i = 0; i < report_num; i++) {
		id = (packet[3*i+3] & 0x07);
		abs_y = (((packet[3*i + 3] & 0x30) << 4) | packet[3*i + 1]);
		abs_x = (((packet[3*i + 3] & 0xC0) << 2) | packet[3*i + 2]);
		if (i == (MAX_FINGERS - 1)) {
			tip = (packet[14] & (0x01 << 1));
		} else {
			tip = (packet[3*report_num + 1] & (0x01 << (2*i+1)));
		}
		input_mt_slot(psmouse->dev, id);
		input_mt_report_slot_state(psmouse->dev, MT_TOOL_FINGER, tip);
		if (tip) {
			fingers++;
			input_report_abs(psmouse->dev, ABS_MT_POSITION_Y, abs_y);
			input_report_abs(psmouse->dev, ABS_MT_POSITION_X, abs_x);
		}
	}
	input_mt_drop_unused(psmouse->dev);
	input_mt_report_pointer_emulation(psmouse->dev, false);
	input_mt_report_finger_count(psmouse->dev, fingers);

	if (priv->type == PIXART_TYPE_CLICKPAD) {
		input_report_key(psmouse->dev, BTN_LEFT, (packet[0] & 0x03));
	} else {
		input_report_key(psmouse->dev, BTN_LEFT, (packet[0] & 0x01));
		input_report_key(psmouse->dev,  BTN_RIGHT, (packet[0] & 0x02));
	}

	input_mt_report_pointer_emulation(psmouse->dev, true);
	input_sync(psmouse->dev);
}

static psmouse_ret_t pixart_process_abs_packet(struct psmouse *psmouse)
{
	unsigned char *packet = psmouse->packet;
	unsigned char curr_num;

	curr_num = ((packet[0] >> 4) & 0x07);
	if (curr_num == MAX_FINGERS && psmouse->pktcnt < psmouse->pktsize) {
		return PSMOUSE_GOOD_DATA;
	} else if (curr_num == 0 && psmouse->pktcnt < 5) {
		return PSMOUSE_GOOD_DATA;
	} else if (psmouse->pktcnt < (3*curr_num + 2)) {
		return PSMOUSE_GOOD_DATA;
	} else if (curr_num > MAX_FINGERS || ((packet[0] & 0x8C) != 0x80)) {
		return PSMOUSE_BAD_DATA;
	}
	pixart_abs_report(psmouse, curr_num);

	return PSMOUSE_FULL_PACKET;
}

static psmouse_ret_t pixart_process_rel_packet(struct psmouse *psmouse)
{
	struct input_dev *dev = psmouse->dev;
	unsigned char *packet = psmouse->packet;

	if (psmouse->pktcnt < psmouse->pktsize) {
		return PSMOUSE_GOOD_DATA;
	}

	input_report_key(dev, BTN_LEFT,    packet[0]       & 1);
	input_report_key(dev, BTN_MIDDLE, (packet[0] >> 2) & 1);
	input_report_key(dev, BTN_RIGHT,  (packet[0] >> 1) & 1);

	input_report_rel(dev, REL_X, packet[1] ? (int) packet[1] - (int) ((packet[0] << 4) & 0x100) : 0);
	input_report_rel(dev, REL_Y, packet[2] ? (int) ((packet[0] << 3) & 0x100) - (int) packet[2] : 0);
	input_report_rel(dev, REL_WHEEL, -(signed char) packet[3]);

	input_sync(dev);
	return PSMOUSE_FULL_PACKET;
}


static void pixart_disconnect(struct psmouse *psmouse)
{
	psmouse_info(psmouse, "Device disconnect");
	pixart_reset(psmouse);
	kfree(psmouse->private);
	psmouse->private = NULL;
}

static int pixart_reconnect(struct psmouse *psmouse)
{
	struct pixart_data *priv = psmouse->private;

	psmouse_info(psmouse, "Device reconnect");
	pixart_reset(psmouse);
	priv->mode = pixart_mode_detect(psmouse);
	if (priv->mode < 0) {
		psmouse_err(psmouse, "Unable to detect the PixArt device\n");
		return -1;
	}
	if (priv->mode == PIXART_MODE_ABS) {
		ps2_command(&psmouse->ps2dev, NULL, PIXART_CMD_SWITCH_PROTOCOL);
	}
	return 0;
}

static void pixart_set_input_params(struct psmouse *psmouse)
{
	struct input_dev *dev = psmouse->dev;
	struct pixart_data *priv = psmouse->private;

	switch (priv->mode) {
	default:
	case (PIXART_MODE_REL):
		__set_bit(EV_KEY, dev->evbit);
		__set_bit(EV_REL, dev->evbit);
		__set_bit(BTN_LEFT, dev->keybit);
		__set_bit(BTN_RIGHT, dev->keybit);

		__set_bit(REL_X, dev->relbit);
		__set_bit(REL_Y, dev->relbit);
		__set_bit(REL_WHEEL, dev->relbit);
		__set_bit(INPUT_PROP_POINTER, dev->propbit);

		psmouse->protocol_handler = pixart_process_rel_packet;
		psmouse->pktsize = 4;
		break;
	case (PIXART_MODE_ABS):
		priv->x_max = MODULE_LOGICAL_MAX_X;
		priv->y_max = MODULE_LOGICAL_MAX_Y;

		__clear_bit(EV_REL, dev->evbit);
		__clear_bit(REL_X, dev->relbit);
		__clear_bit(REL_Y, dev->relbit);
		__clear_bit(BTN_MIDDLE, dev->keybit);

		__set_bit(EV_KEY, dev->evbit);
		__set_bit(BTN_LEFT, dev->keybit);
		__set_bit(BTN_RIGHT, dev->keybit);
		if (priv->type == PIXART_TYPE_CLICKPAD) {
			__set_bit(INPUT_PROP_BUTTONPAD, dev->propbit);
			__clear_bit(BTN_RIGHT, dev->keybit);
		}
		__set_bit(BTN_TOUCH, dev->keybit);
		__set_bit(BTN_TOOL_FINGER, dev->keybit);
		__set_bit(BTN_TOOL_DOUBLETAP, dev->keybit);
		__set_bit(BTN_TOOL_TRIPLETAP, dev->keybit);
		__set_bit(BTN_TOOL_QUADTAP, dev->keybit);
		__set_bit(INPUT_PROP_POINTER, dev->propbit);

		__set_bit(EV_ABS, dev->evbit);
		input_set_abs_params(dev, ABS_X, 0, priv->x_max, 0, 0);
		input_set_abs_params(dev, ABS_Y, 0, priv->y_max, 0, 0);

		input_set_abs_params(dev, ABS_MT_POSITION_X, 0, priv->x_max, 0, 0);
		input_set_abs_params(dev, ABS_MT_POSITION_Y, 0, priv->y_max, 0, 0);

		input_mt_init_slots(dev, SLOTS_NUMBER,  0);

		psmouse->protocol_handler = pixart_process_abs_packet;
		psmouse->pktsize = 15;
		break;
	}
}

int pixart_detect(struct psmouse *psmouse, bool set_properties)
{
	enum PIXART_MODE mode;
	enum PIXART_TYPE type;

	pixart_reset(psmouse);

	type = pixart_type_detect(psmouse);
	if (type < 0)
		return -ENODEV;

	mode = pixart_mode_detect(psmouse);
	if (mode < 0)
		return -ENODEV;
	psmouse_info(psmouse, "Detect PixArt Device");

	if (set_properties) {
		psmouse->vendor = "PixArt";
		switch (type) {
		default:
		case (PIXART_TYPE_CLICKPAD):
			psmouse->name = "clickpad";
			break;
		case (PIXART_TYPE_NORMALPAD):
			psmouse->name = "touchpad";
			break;
		}
	}
	return (0);
}

int pixart_init(struct psmouse *psmouse)
{
	struct pixart_data *priv;
	psmouse->private = priv = kzalloc(sizeof(struct pixart_data),
					  GFP_KERNEL);
	if (!priv) {
		return -ENOMEM;
	}
	pixart_reset(psmouse);

	priv->type = pixart_type_detect(psmouse);
	if (priv->type < 0) {
		goto fail;
	}

	priv->mode = pixart_mode_detect(psmouse);
	if (priv->mode < 0) {
		psmouse_err(psmouse, "Unable to initialize the PixArt device\n");
		goto fail;
	}
	if (priv->mode == PIXART_MODE_ABS) {
		ps2_command(&psmouse->ps2dev, NULL, PIXART_CMD_SWITCH_PROTOCOL);
	} else {
		pixart_reset(psmouse);
		pixart_intellimouse_detect(psmouse);
	}

	psmouse_info(psmouse, "Set device as Type: %x, mode: %x", priv->type, priv->mode);

	pixart_set_input_params(psmouse);

	psmouse->disconnect = pixart_disconnect;
	psmouse->reconnect = pixart_reconnect;
	psmouse->cleanup = pixart_reset;

	psmouse->resync_time = 0;
	return 0;

fail:
	pixart_reset(psmouse);
	kfree(priv);
	psmouse->private = NULL;
	return -1;
}



