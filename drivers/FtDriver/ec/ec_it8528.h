/*
 * EC (Embedded Controller) IT8528 device driver header for phytium czc B20
 *
 * EC relative header file. All the EC registers should be defined here.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at you option) and later version.
 */

#ifndef __EC_IT8528_H__
#define __EC_IT8528_H__

#define EC_VERSION		"1.0"


#define EC_EVENT_BIT        (1 << 11)
#define I8042_KEY_BIT       (1 << 1)
#define I8042_TOUCH_BIT     (1 << 12)

#define LPC_STATUS_REG		0xF4
#define LPC_INTERRUPT_REG	0xF0


/* This spinlock is dedicated for 62&66 ports and super io port access. */
extern spinlock_t i8042_lock;
extern spinlock_t index_access_lock;

/*
 * The following registers are determined by the EC index configureation.
 * 1. fill the PORT_INDEX as EC register.
 * 2. fill the PORT_DATA as EC register write data or get the data from it.
 */
/* base address for io access for Notebook platform */
#define SIO_INDEX_PORT		0x4E
#define SIO_DATA_PORT		0x4F

/*
 * EC delay time 500us for register and status access
 * Unit : us
 */
#define EC_REG_DELAY        30000
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

/*
 * ACPI OEM commands.
 */
#define CMD_RESET			0x4E	/* Reset and poweroff the machine auto-clear: rd/wr */
enum
{
	RESET_OFF = 0,
	RESET_ON,
	PWROFF_ON,
	STR_ON,
	STANDBY_ON
};

#define CMD_EC_VERSION		0x4F	/* EC Version OEM command: 36 Bytes */

/*
 * Used ACPI legacy command 80h to do active.
 */
/* >>> Read/Write temperature & fan index for ACPI 80h/81h command. */
#define INDEX_CPU_TEMP_VALUE		0x98	/* Current CPU temperature value, Read and Write(81h command). */
#define INDEX_GPU_TEMP_VALUE		0x9A	/* Current GPU temperature value, Read and Write(81h command). */

#define INDEX_FAN_MAXSPEED_LEVEL	0x5B	/* Fan speed maxinum levels supported. Defaut is 6. */
#define INDEX_FAN_SPEED_LEVEL		0x5C	/* FAn speed level. [0,5] or [0x06, 0x38]*/
#define INDEX_FAN_CTRLMOD			0x5D	/* Fan control mode, 0 = by EC, 1 = by Host.*/
enum
{
	FAN_CTRL_BYEC = 0,
	FAN_CTRL_BYHOST
};
#define INDEX_FAN_STSCTRL			0x5E	/* Fan status/control, 0 = stop, 1 = run. */
enum
{
	FAN_STSCTRL_OFF = 0,
	FAN_STSCTRL_ON
};
#define INDEX_FAN_ERRSTS			0x5F	/* Fan error status, 0 = no error, 1 = has error. */
enum
{
	FAN_ERRSTS_NO = 0,
	FAN_ERRSTS_HAS
};

#define INDEX_CPU_FAN_SPEED_LOW			0xF2	/* Fan speed low byte.*/
#define INDEX_CPU_FAN_SPEED_HIGH		0xF3	/* Fan speed high byte. */
#define INDEX_GPU_FAN_SPEED_LOW			0xF4	/* Fan speed low byte.*/
#define INDEX_GPU_FAN_SPEED_HIGH		0xF5	/* Fan speed high byte. */
/* <<< End Temp & Fan */

/* >>> Read/Write LCD backlight information/control index for ACPI 80h/81h command. */
#define INDEX_BACKLIGHT_CTRLMODE	0x57	/* LCD backlight control mode: 0 = by EC, 1 = by HOST */
enum
{
	BACKLIGHT_CTRL_BYEC = 0,
	BACKLIGHT_CTRL_BYHOST
};
#define INDEX_BACKLIGHT_STSCTRL		0x58	/* LCD backlight status or control: 0 = turn off, 1 = turn on */
enum
{
	BACKLIGHT_OFF = 0,
	BACKLIGHT_ON
};
#define	INDEX_DISPLAY_MAXBRIGHTNESS_LEVEL	0x59	/* LCD backlight brightness max level */
#define	INDEX_DISPLAY_BRIGHTNESS_SET	0x0E	/* 10 stages (0~9) LCD backlight brightness adjust */
#define	INDEX_DISPLAY_BRIGHTNESS_GET	0x0F	/* 10 stages (0~9) LCD backlight brightness adjust */

enum
{
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_0	= 0,	/* This level is backlight turn off. */
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_1,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_2,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_3,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_4,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_5,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_6,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_7,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_8,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_9,
	FLAG_DISPLAY_BRIGHTNESS_LEVEL_10
};
/* <<< End Backlight */

/* >>> Read battery index command */
/*
 * The reported battery die temperature.
 * The temperature is expressed in units of 0.25 seconds and is updated every 2.56 seconds.
 * The equation to calculate reported pack temperature is:
 * Temperature = 0.1 * (256 * TEMPH + TEMPL) Kelvin
 * Temperature -= 273 Degrees Celsius
 * The host sytem has read-only access to this register pair.
 */
#define INDEX_BATTERY_VOL_LOW		0x2A	/* Battery Voltage Low byte. */
#define INDEX_BATTERY_VOL_HIGH		0x2B	/* Battery Voltage High byte. */
#define INDEX_BATTERY_CURRENT_LOW	0x2C	/* Battery Current Low byte. */
#define INDEX_BATTERY_CURRENT_HIGH	0x2D	/* Battery Current High byte. */
#define INDEX_BATTERY_AC_LOW		0x2E	/* Battery AverageCurrent Low byte. */
#define INDEX_BATTERY_AC_HIGH		0x2F	/* Battery AverageCurrent High byte. */
#define INDEX_BATTERY_CAPACITY		0x21	/* Battery RemainingCapacity percent. */
#define INDEX_BATTERY_STATUS_LOW	0x3C	/* Battery Status low byte. */

enum
{
	BIT_BATTERY_STATUS_FD = 4,	/* Battery Fully Discharged Notify. 1 = Fully Discharged */
	BIT_BATTERY_STATUS_FC,		/* Battery Fully Charged Notify. 1 = Fully Charged. */
	BIT_BATTERY_STATUS_DSG,		/* Battery Discharging mode. 0 = in charging mode, 1 = in discharging mode,
	                               relaxation mode, or valid charge termination has occurred. */
	BIT_BATTERY_STATUS_INIT		/* Battery Initialization. 1 = Initialization */
};
#define INDEX_BATTERY_STATUS	0x3C	/* Battery Status byte. */
enum
{
	BIT_POWER_BATEMPTY = 4,		/* Battery in empty status. */
	BIT_POWER_BATFCHG = 5,		/* Battery in fully charging status. */
	BIT_POWER_BATCHG = 6,		/* Battery in charging status. */
};

#define INDEX_BATTERY_CAP_LOW		0x24    /* Battery Capacity Low byte. */
#define INDEX_BATTERY_CAP_HIGH		0x25    /* Battery Capacity High byte. */
#define INDEX_BATTERY_RC_LOW		0x26	/* Battery RemainingCapacity Low byte. */
#define INDEX_BATTERY_RC_HIGH		0x27	/* Battery RemainingCapacity High byte. */
#define INDEX_BATTERY_ATTE_LOW	0x9C	/* Battery AverageTimeToEmpty Low byte. */
#define INDEX_BATTERY_ATTE_HIGH	0x9D	/* Battery AverageTimeToEmpty High byte. */
#define INDEX_BATTERY_ATTF_LOW	0xA4	/* Battery AverageTimeToFull Low byte. */
#define INDEX_BATTERY_ATTF_HIGH	0xA5	/* Battery AverageTimeToFull High byte. */
#define INDEX_BATTERY_FCC_LOW	0x24	/* Battery FullChargeCapacity Low byte. */
#define INDEX_BATTERY_FCC_HIGH	0x25	/* Battery FullChargeCapacity High byte. */
#define INDEX_BATTERY_CC_LOW		0xAC	/* Battery ChargingCurrent Low byte. */
#define INDEX_BATTERY_CC_HIGH		0xAD	/* Battery ChargingCurrent High byte. */
#define INDEX_BATTERY_CV_LOW		0xAA	/* Battery ChargingVoltage Low byte. */
#define INDEX_BATTERY_CV_HIGH		0xAB	/* Battery ChargingVoltage High byte. */
#define INDEX_BATTERY_TEMP_LOW	0x28	/* Battery temperature low byte. */
#define INDEX_BATTERY_TEMP_HIGH	0x29	/* Battery temperature high byte. */
#define INDEX_BATTERY_CYCLECNT_LOW	0xA8	/* Battery CycleCount Low byte. */
#define INDEX_BATTERY_CYCLECNT_HIGH 0xA9	/* Battery CycleCount High byte. */

/* Battery static information. */
#define INDEX_BATTERY_DC_LOW		0x38	/* Battery DesignCapacity Low byte. */
#define INDEX_BATTERY_DC_HIGH		0x39 	/* Battery DesignCapacity High byte. */
#define INDEX_BATTERY_DV_LOW		0x3A	/* Battery DesignVoltage Low byte. */
#define INDEX_BATTERY_DV_HIGH		0x3B	/* Battery DesignVoltage High byte. */
#define INDEX_BATTERY_SN_LOW		0x3E	/* Battery SerialNumber Low byte. */
#define INDEX_BATTERY_SN_HIGH		0x3F	/* Battery SerialNumber High byte. */
/* <<< End Battery */

/* Phytium S3 timeout information */
#define INDEX_POWERON_TYPE		0x0B   /* Phytium EC Poweron type. */
#define INDEX_S3_TIMEOUT_LO		0x0C   /* Phytium S3 timeout Low byte. */
#define INDEX_S3_TIMEOUT_HI		0x0D   /* Phytium S3 timeout High byte. */

#define PHYTIUM_S3_TIMEOUT_DEFAULT	10	/* bxc timeout default 10 min */
#define PHYTIUM_S3_TIMEOUT_FOREVER	0x0

#define PHYTIUM_S3_TIMEOUT_TYPE		0xAA
#define PHYTIUM_S3_NORMAL_WAKEUP_TYPE	0x55
/* <<< End Phytium S3 */

#define MASK(x)	(1 << x)

#define INDEX_POWER_STATUS		0xB0	/* Read current power status. */
enum
{
	BIT_POWER_ACPRES,		/* AC present. */
	BIT_POWER_BATPRES,		/* ZW000B Master Battery present. */
};

#define	INDEX_DEVICE_STATUS		0x81	/* Read Current Device Status */
enum
{
	BIT_DEVICE_TP = 0,	/* TouchPad status: 0 = close, 1 = open */
	BIT_DEVICE_WLAN,	/* WLAN status: 0 = close, 1 = open */
	BIT_DEVICE_3G,		/* 3G status: 0 = close, 1 = open */
	BIT_DEVICE_CAM,		/* Camera status: 0 = close, 1 = open */
	BIT_DEVICE_MUTE,	/* Mute status: 0 = close, 1 = open */
	BIT_DEVICE_LID,		/* LID status: 0 = close, 1 = open */
	BIT_DEVICE_BKLIGHT,	/* BackLight status: 0 = close, 1 = open */
	BIT_DEVICE_SIM		/* SIM Card status: 0 = pull out, 1 = insert */
};

#define	INDEX_SHUTDOWN_ID		0x82	/* Read Shutdown ID */
enum
{
	BIT_SHUTDNID_S45 = 0,	/* in S4 or S5 */
	BIT_SHUTDNID_BATDEAD,	/* Battery Dead */
	BIT_SHUTDNID_OVERHEAT,	/* Over Heat */
	BIT_SHUTDNID_SYSCMD,	/* System command */
	BIT_SHUTDNID_LPRESSPWN,	/* Long press power button */
	BIT_SHUTDNID_PWRUNDER9V,/* Batery voltage low under 9V */
	BIT_SHUTDNID_S3,		/* Entry S3 state */
	BIT_SHUTDNID_S1			/* Entry S1 state */
};

#define	INDEX_SYSTEM_CFG		0x82		/* Read System config */
#define BIT_SYSCFG_TPSWITCH		(1 << 0)	/* TouchPad switch */
#define BIT_SYSCFG_WLANPRES		(1 << 1)	/* WLAN present */
#define BIT_SYSCFG_NB3GPRES		(1 << 2)	/* 3G present */
#define BIT_SYSCFG_CAMERAPRES	(1 << 3)	/* Camera Present */
#define BIT_SYSCFG_VOLCTRLEC	(1 << 4)	/* Volume control by EC */
#define BIT_SYSCFG_BLCTRLEC		(1 << 5)	/* Backlight control by EC */
#define BIT_SYSCFG_AUTOBRIGHT	(1 << 7)	/* Auto brightness */

#define	INDEX_VOLUME_LEVEL		0xA6		/* Read Volume Level command */
#define	INDEX_VOLUME_MAXLEVEL	0xA7		/* Volume MaxLevel */
#define	VOLUME_MAX_LEVEL		0x0A		/* Volume level max is 11 */
enum
{
	FLAG_VOLUME_LEVEL_0 = 0,
	FLAG_VOLUME_LEVEL_1,
	FLAG_VOLUME_LEVEL_2,
	FLAG_VOLUME_LEVEL_3,
	FLAG_VOLUME_LEVEL_4,
	FLAG_VOLUME_LEVEL_5,
	FLAG_VOLUME_LEVEL_6,
	FLAG_VOLUME_LEVEL_7,
	FLAG_VOLUME_LEVEL_8,
	FLAG_VOLUME_LEVEL_9,
	FLAG_VOLUME_LEVEL_10
};

/* Camera control */
#define INDEX_CAM_STSCTRL			0xAA
enum
{
	CAM_STSCTRL_OFF = 0,
	CAM_STSCTRL_ON
};

#define	INDEX_CPU_TEMP		0x1E		/* Read CPU temperature */
#define	INDEX_GPU_TEMP		0x10		/* Read GPU temperature */

/* EC_SC input */
/* EC Status query, by direct read 66h port. */
#define EC_SMI_EVT		(1 << 6)	/* 1 = SMI event padding */
#define EC_SCI_EVT		(1 << 5)	/* 1 = SCI event padding */
#define EC_BURST		(1 << 4)	/* 1 = Controller is in burst mode */
#define EC_CMD			(1 << 3)	/* 1 = Byte in data register is command */

#define EC_IBF			(1 << 1)	/* 1 = Input buffer full (data ready for ec) */
#define EC_OBF			(1 << 0)	/* 1 = Output buffer full (data ready for host) */

/* LPC Event Number from EC */
enum
{
	EC_EVENT_NUM_BAT = 0xB3,		/* BAT in/out */
	EC_EVENT_NUM_AC = 0xB4,			/* AC in/out */
	EC_EVENT_NUM_POWERBTN = 0xB5,		/* power button */
	EC_EVENT_NUM_LID =	0xD0,		/* press the lid or not */
	EC_EVENT_NUM_RES1 = 0xD1,		/* reserve event1 */
	EC_EVENT_NUM_RES2 = 0xD2,		/* reserve event2 */
	EC_EVENT_NUM_SLEEP = 0x40,		/* 0x40, Fn+F4 for entering sleep mode */
	EC_EVENT_NUM_WLAN = 0x35,		/* 0x41, Fn+F6, Wlan is on or off */
	EC_EVENT_NUM_TP ,			/* 0x43, Fn+F5, TouchPad is on */
	EC_EVENT_NUM_SCREENLOCK ,		/* 0x43, Fn+F5, ScreenLock */
	EC_EVENT_NUM_BRIGHTNESS_OFF,		/* 0x44, Fn+F3, LCD backlight brightness on or off */
	EC_EVENT_NUM_BRIGHTNESS_DN,		/* 0x45, Fn+F1, LCD backlight brightness down adjust */
	EC_EVENT_NUM_BRIGHTNESS_UP,		/* 0x46, Fn+F2, LCD backlight brightness up adjust */
	EC_EVENT_NUM_DISPLAY_TOGGLE,		/* 0x47, Fn+F7, Video switch(LCD/HDMI...)*/
	EC_EVENT_NUM_MICMUTE,			/* 0xc2, microphone mute */
	EC_EVENT_NUM_CPU_TEMP,			/* 0x3b, read out cpu temperature*/
	EC_EVENT_NUM_RFKILL			/* 0xbd, airplane mode switch */
};

#define EC_EVENT_NUM_MIN EC_EVENT_NUM_BAT
#define EC_EVENT_NUM_MAX EC_EVENT_NUM_RES2
#define EC_EVENT_NUM_GSENSOR EC_EVENT_NUM_RES2

#define	INDEX_LID_STATUS		0x46 	    /* Lid status register */
#define	LID_STATUS_BIT		(1 << 0) 	/* Lid status BIT0  1: Closed, 0: Open */

typedef int (*ec_handler)(int status);

extern bool it8528_get_ec_ibf_flags(void);
extern bool it8528_get_ec_obf_flags(void);
extern bool it8528_get_ec_evt_flags(void);

/* The general ec index-io port read action */
extern void clean_it8528_event_status(void);
extern unsigned char it8528_read(unsigned char index);
extern unsigned char it8528_read_all(unsigned char command, unsigned char index);
extern unsigned char it8528_read_noindex(unsigned char command);

/* The general ec index-io port write action */
extern int it8528_write(unsigned char index, unsigned char data);
extern int it8528_write_all(unsigned char command, unsigned char index, unsigned char data);
extern int it8528_write_noindex(unsigned char command, unsigned char data);

/* Query sequence of 62/66 port access routine. */
extern int it8528_query_seq(unsigned char command);
extern int it8528_get_event_num(void);
extern void it8528_ec_event_int_enable(void);
extern int it8528_query_clean_event(void);

/* The ec interrupt functions */
extern int it8528_init(void);
extern int lpc_ec_interrupt_occurs(void);
extern void lpc_ec_interrupt_clear(void);
extern void lpc_interrupt_clear_all(void);

#endif /* __EC_IT8528_H__ */
