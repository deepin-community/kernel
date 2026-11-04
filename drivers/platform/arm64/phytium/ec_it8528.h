/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __EC_IT8528_H__
#define __EC_IT8528_H__

#include <linux/platform_device.h>
#include "../../../bus/phytium_pio.h"

extern spinlock_t i8042_lock;

/*
 * EC delay time 500us for register and status access
 * Unit : us
 */
#define EC_CMD_TIMEOUT      0x1000
#define EC_SEND_TIMEOUT     0xffff
#define EC_RECV_TIMEOUT     0xffff

/*
 * EC access port for with Host communication.
 */
#define EC_CMD_PORT			0x66
#define EC_STS_PORT			0x66
#define EC_DAT_PORT			0x62

/*
 * ACPI legacy commands.
 */
#define CMD_READ_EC         0x80	 /* Read EC command. */
#define CMD_WRITE_EC        0x81	 /* Write EC command. */
#define CMD_GET_EVENT_NUM   0x84	 /* Query EC command, for get ec event number. */
#define CMD_ENABLE_EVENT_EC 0x86	 /* Enable EC event interrupt. */
#define CMD_DISABLE_EVENT_EC 0x87	 /* Disable EC event interrupt. */

/*
 * Used ACPI legacy command 80h to do active.
 */
	/* Current GPU temperature value, Read and Write(81h command). */
#define INDEX_GPU_TEMP_VALUE		0x9A

#define INDEX_CPU_FAN_SPEED_LOW			0xF2	/* Fan speed low byte.*/
#define INDEX_CPU_FAN_SPEED_HIGH		0xF3	/* Fan speed high byte. */
#define INDEX_GPU_FAN_SPEED_LOW			0xF4	/* Fan speed low byte.*/
#define INDEX_GPU_FAN_SPEED_HIGH		0xF5	/* Fan speed high byte. */

	/* 10 stages (0~9) LCD backlight brightness adjust */
#define	INDEX_DISPLAY_BRIGHTNESS_SET	0x0E
	/* 10 stages (0~9) LCD backlight brightness adjust */
#define	INDEX_DISPLAY_BRIGHTNESS_GET	0x0F

/* >>> Read battery index command */
/*
 * The reported battery die temperature.
 * The equation to calculate reported pack temperature is:
 * Temperature = 0.1 * (256 * TEMPH + TEMPL) Kelvin
 * Temperature -= 273 Degrees Celsius
 * The host system has read-only access to this register pair.
 */
#define INDEX_BATTERY_TEMP_LOW	0x28	/* Battery temperature low byte. */
#define INDEX_BATTERY_TEMP_HIGH	0x29	/* Battery temperature high byte. */
#define INDEX_BATTERY_VOL_LOW		0x2A	/* Battery Voltage Low byte. */
#define INDEX_BATTERY_VOL_HIGH		0x2B	/* Battery Voltage High byte. */
#define INDEX_BATTERY_CURRENT_LOW	0x2C	/* Battery Current Low byte. */
#define INDEX_BATTERY_CURRENT_HIGH	0x2D	/* Battery Current High byte. */
#define INDEX_BATTERY_AC_LOW		0x2E	/* Battery AverageCurrent Low byte. */
#define INDEX_BATTERY_AC_HIGH		0x2F	/* Battery AverageCurrent High byte. */
#define INDEX_BATTERY_CAPACITY		0x21	/* Battery RemainingCapacity percent. */
#define INDEX_BATTERY_STATUS	0x3C	/* Battery Status byte. */
enum {
	BIT_POWER_BATFCHG = 5,		/* Battery in fully charging status. */
	BIT_POWER_BATCHG = 6,		/* Battery in charging status. */
};

#define INDEX_BATTERY_CAP_LOW		0x24    /* Battery Capacity Low byte. */
#define INDEX_BATTERY_CAP_HIGH		0x25    /* Battery Capacity High byte. */
#define INDEX_BATTERY_RC_LOW		0x26	/* Battery RemainingCapacity Low byte. */
#define INDEX_BATTERY_RC_HIGH		0x27	/* Battery RemainingCapacity High byte. */
#define INDEX_BATTERY_FCC_LOW	0x24	/* Battery FullChargeCapacity Low byte. */
#define INDEX_BATTERY_FCC_HIGH	0x25	/* Battery FullChargeCapacity High byte. */
#define INDEX_BATTERY_CV_LOW		0xAA	/* Battery ChargingVoltage Low byte. */
#define INDEX_BATTERY_CV_HIGH		0xAB	/* Battery ChargingVoltage High byte. */
#define INDEX_BATTERY_CYCLECNT_LOW	0xA8	/* Battery CycleCount Low byte. */
#define INDEX_BATTERY_CYCLECNT_HIGH 0xA9	/* Battery CycleCount High byte. */

/* Battery static information. */
#define INDEX_BATTERY_DC_LOW		0x38	/* Battery DesignCapacity Low byte. */
#define INDEX_BATTERY_DC_HIGH		0x39	/* Battery DesignCapacity High byte. */
#define INDEX_BATTERY_DV_LOW		0x3A	/* Battery DesignVoltage Low byte. */
#define INDEX_BATTERY_DV_HIGH		0x3B	/* Battery DesignVoltage High byte. */
#define INDEX_BATTERY_SN_LOW		0x3E	/* Battery SerialNumber Low byte. */
#define INDEX_BATTERY_SN_HIGH		0x3F	/* Battery SerialNumber High byte. */
/* <<< End Battery */

#define MASK(x)	(1 << x)

#define INDEX_POWER_STATUS		0xB0	/* Read current power status. */
enum {
	BIT_POWER_ACPRES,		/* AC present. */
	BIT_POWER_BATPRES,		/* ZW000B Master Battery present. */
};

/* EC_SC input */
/* EC Status query, by direct read 66h port. */
#define EC_SCI_EVT		(1 << 5)	/* 1 = SCI event padding */

#define EC_IBF			(1 << 1)	/* 1 = Input buffer full (data ready for ec) */
#define EC_OBF			(1 << 0)	/* 1 = Output buffer full (data ready for host) */

/* LPC Event Number from EC */
enum {
	EC_EVENT_NUM_BAT = 0xB3,		/* BAT in/out */
	EC_EVENT_NUM_AC = 0xB4,			/* AC in/out */
	EC_EVENT_NUM_POWERBTN = 0xB5,		/* power button */
	EC_EVENT_NUM_LID =	0xD0,		/* press the lid or not */
	EC_EVENT_NUM_RES2 = 0xD2,		/* reserve event2 */
	EC_EVENT_NUM_SLEEP = 0x40,		/* 0x40, Fn+F4 for entering sleep mode */
	EC_EVENT_NUM_WLAN = 0x35,		/* 0x41, Fn+F6, Wlan is on or off */
	EC_EVENT_NUM_TP,			/* 0x43, Fn+F5, TouchPad is on */
	EC_EVENT_NUM_SCREENLOCK,		/* 0x43, Fn+F5, ScreenLock */
			/* 0x44, Fn+F3, LCD backlight brightness on or off */
	EC_EVENT_NUM_BRIGHTNESS_OFF,
			/* 0x45, Fn+F1, LCD backlight brightness down adjust */
	EC_EVENT_NUM_BRIGHTNESS_DN,
			/* 0x46, Fn+F2, LCD backlight brightness up adjust */
	EC_EVENT_NUM_BRIGHTNESS_UP,
	EC_EVENT_NUM_DISPLAY_TOGGLE,		/* 0x47, Fn+F7, Video switch(LCD/HDMI...)*/
	EC_EVENT_NUM_MICMUTE,			/* 0xc2, microphone mute */
	EC_EVENT_NUM_RFKILL			/* 0xbd, airplane mode switch */
};

#define EC_EVENT_NUM_GSENSOR EC_EVENT_NUM_RES2

/* Upper bound on the events consumed per interrupt or drain pass. */
#define EC_EVENT_DRAIN_MAX	16

#define	INDEX_LID_STATUS		0x46	    /* Lid status register */
#define	LID_STATUS_BIT		(1 << 0)	/* Lid status BIT0  1: Closed, 0: Open */

extern bool it8528_get_ec_evt_flags(void);

/* The general ec index-io port read action */
extern int it8528_read_byte(unsigned char index, u8 *value);
extern int it8528_read_word(unsigned char high_index,
			    unsigned char low_index, u16 *value);

/* The general ec index-io port write action */
extern int it8528_write(unsigned char index, unsigned char data);

/* Query sequence of 62/66 port access routine. */
extern int it8528_query_get_event_num(u8 *event);
extern void it8528_ec_event_int_enable(void);
extern void it8528_ec_event_int_disable(void);
extern int it8528_query_clean_event(void);

/* The ec interrupt functions */
extern int it8528_init(void);

/* EC sensor/hwmon registration */
extern void ec_sensors_register(struct platform_device *pdev);

#endif /* __EC_IT8528_H__ */
