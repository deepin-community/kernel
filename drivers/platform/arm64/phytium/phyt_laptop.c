// SPDX-License-Identifier: GPL-2.0
#include <linux/io.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/backlight.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/power_supply.h>
#include <linux/input.h>
#include <linux/input/sparse-keymap.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/string.h>
#include "ec_it8528.h"
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/cputypes.h>
#include <linux/classtypes.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>

static inline u16 ec_read16(u8 high, u8 low)
{
	return (it8528_read(high) << 8) | it8528_read(low);
}


static bool ft_display_state = true;
static unsigned int ft_display_level;
static int ft_set_display_on(bool on);
static int ec_gsonsor_init(struct platform_device *pdev);

#define EC_LPC_DEV	"ec"


/* Power supply */
enum {
	APM_AC_OFFLINE =	0,
	APM_AC_ONLINE,
	APM_AC_BACKUP,
	APM_AC_UNKNOWN =	0xff
};

enum {
	APM_BAT_STATUS_HIGH = 0,
	APM_BAT_STATUS_LOW,
	APM_BAT_STATUS_CRITICAL,
	APM_BAT_STATUS_CHARGING,
	APM_BAT_STATUS_NOT_PRESENT,
	APM_BAT_STATUS_UNKNOWN =	0xff
};

enum /* bat_reg_flag */
{
	BAT_REG_TEMP_FLAG = 1,
	BAT_REG_VOLTAGE_FLAG,
	BAT_REG_CURRENT_FLAG,
	BAT_REG_AC_FLAG,
	BAT_REG_RC_FLAG,
	BAT_REG_FCC_FLAG,
	BAT_REG_ATTE_FLAG,
	BAT_REG_ATTF_FLAG,
	BAT_REG_RSOC_FLAG,
	BAT_REG_CYCLCNT_FLAG
};

/* Power info cached timeout */
#define POWER_INFO_CACHED_TIMEOUT	100	/* jiffies */

/* Backlight */
#define MAX_BRIGHTNESS	100

/* Screen rotation */
#define EC_GSENSOR_X_HIGH		0X63
#define EC_GSENSOR_X_LOW		0X64
#define EC_GSENSOR_Y_HIGH		0X65
#define EC_GSENSOR_Y_LOW		0X66
#define EC_GSENSOR_Z_HIGH		0X67
#define EC_GSENSOR_Z_LOW		0X68

/* Power information structure */
struct ft_power_info {
	/* AC insert or not */
	unsigned int ac_in;
	/* Battery insert or not */
	unsigned int bat_in;
	unsigned int health;

	/* Battery designed capacity */
	unsigned int design_capacity;
	/* Battery designed voltage */
	unsigned int design_voltage;
	/* Battery capacity after full charged */
	unsigned int full_charged_capacity;
	/* Battery Manufacture Date */
	unsigned char manufacture_date[11];
	/* Battery Serial number */
	unsigned char serial_number[8];
	/* Battery Manufacturer Name, max 11 + 1(length) bytes */
	unsigned char manufacturer_name[12];
	/* Battery Device Name, max 7 + 1(length) bytes */
	unsigned char device_name[8];
	/* Battery Technology */
	unsigned int technology;
	/* Battery cell count */
	unsigned char cell_count;

	/* Battery dynamic charge/discharge voltage */
	unsigned int voltage_now;
	/* Battery dynamic charge/discharge average current */
	int current_now;
	int current_sign;
	int current_average;
	/* Battery current remaining capacity */
	unsigned int remain_capacity;
	/* Battery current remaining capacity percent */
	unsigned int remain_capacity_percent;
	/* Battery current temperature */
	unsigned int temperature;
	/* Battery current remaining time (AverageTimeToEmpty) */
	unsigned int remain_time;
	/* Battery current full charging time (averageTimeToFull) */
	unsigned int fullchg_time;
	/* Battery Status */
	unsigned int charge_status;
	/* Battery current cycle count (CycleCount) */
	unsigned int cycle_count;
};

/* LPC device structure */
struct lpc_device {
	/* The lpc number get from ec */
	unsigned char number;
	/* count */
	unsigned char parameter;
	/* Irq relative */
	unsigned char irq;
	unsigned char irq_data;
	/* Device name */
	unsigned char name[10];
};


/* LPC device object */
static struct lpc_device *ft_lpc_device;
static struct lpc_device *ft_gpio_device;
static int ft_lpc_event_probe(struct platform_device *);

static int ft_get_brightness(struct backlight_device *);
static int ft_set_brightness(struct backlight_device *);

/* >>>Power management operation */
/* Update battery information handle function. */
static void ft_power_battery_info_update(unsigned char bat_reg_flag);
/* Clear battery static information. */
static void ft_power_info_battery_static_clear(void);
/* Get battery static information. */
static void ft_power_info_battery_static_update(void);
/* Update power_status value */
static void ft_power_info_power_status_update(void);
static void ft_bat_get_string(unsigned char index, unsigned char *bat_string);
/* Power supply Battery get property handler */
static int ft_bat_get_property(struct power_supply *pws,
			enum power_supply_property psp, union power_supply_propval *val);
/* Power supply AC get property handler */
static int ft_ac_get_property(struct power_supply *pws,
			enum power_supply_property psp, union power_supply_propval *val);
/* LPC device AC event handler */
static void ft_ac_handler(int status);
/* LPC device Battery event handler */
static void ft_bat_handler(int status);

/* <<<End power management operation */

/* LPC device LID event handler */
static void ft_lid_handler(int status);

/* Platform device suspend handler */
static int ft_laptop_suspend(struct device *dev);
/* Platform device resume handler */
static int ft_laptop_resume(struct device *dev);

/* ec device event structure */
struct ec_event {
	int index;
	ec_handler handler;
};

static struct input_dev *ft_hotkey_dev;
static struct input_dev *ec_gsensor_dev;
static const struct key_entry ft_keymap[] = {
	/* LID event */
	{KE_SW,  EC_EVENT_NUM_LID, {SW_LID}},
	/* Fn+F4, Sleep */
	{KE_KEY, EC_EVENT_NUM_SLEEP, {KEY_SLEEP}},
	/* Fn+F5, toutchpad on/off */
	{KE_KEY, EC_EVENT_NUM_TP, {KEY_TOUCHPAD_TOGGLE}},
	/* Fn+F3, Brightness off */
	{KE_KEY, EC_EVENT_NUM_BRIGHTNESS_OFF, {KEY_DISPLAYTOGGLE} },
	/* Fn+F1, Brightness down */
	{KE_KEY, EC_EVENT_NUM_BRIGHTNESS_DN, {KEY_BRIGHTNESSDOWN}},
	/* Fn+F2, Brightness up */
	{KE_KEY, EC_EVENT_NUM_BRIGHTNESS_UP, {KEY_BRIGHTNESSUP}},
	/*Fn+F7, Dispaly switch */
	{KE_KEY, EC_EVENT_NUM_DISPLAY_TOGGLE, {KEY_SWITCHVIDEOMODE}},
	/*Fn+F6, WLAN on/off */
	{KE_KEY, EC_EVENT_NUM_WLAN, {KEY_WLAN}},
	/* Power button */
	{KE_KEY, EC_EVENT_NUM_POWERBTN, {KEY_POWER}},
	/* screen lock */
	{KE_KEY, EC_EVENT_NUM_SCREENLOCK, {KEY_SCREENLOCK }},
	/* Micphone mute*/
	{KE_KEY, EC_EVENT_NUM_MICMUTE, {KEY_MICMUTE}},
	/* airplane mode switch */
	{KE_KEY, EC_EVENT_NUM_RFKILL, {KEY_RFKILL}},
	{KE_END, 0}
};

static const struct acpi_device_id ec_acpi_match[] = {
	{ "FTEC0001", 0 },
	{ "PHYT000B", 0 },
	{}
};
MODULE_DEVICE_TABLE(acpi, ec_acpi_match);

static SIMPLE_DEV_PM_OPS(gpio_pm_ops, ft_laptop_suspend, ft_laptop_resume);

static struct platform_driver ft_ec_pdriver = {
	.probe = ft_lpc_event_probe,
	.driver = {
		.name = "ec_device",
		.pm     = &gpio_pm_ops,
		.acpi_match_table = ACPI_PTR(ec_acpi_match),
		.owner = THIS_MODULE,
	},
};

/* Backlight device object */
static struct backlight_device *ft_backlight_dev;
/* Backlight device operations table object */
static const struct backlight_ops ft_backlight_ops = {

	.get_brightness = ft_get_brightness,
	.update_status = ft_set_brightness,
};

/* Power info object */
static struct ft_power_info *power_info;
/* Power supply Battery property object */
static enum power_supply_property ft_bat_props[] = {

	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL, /* in uAh */
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY, /* in percents! */
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG,
	POWER_SUPPLY_PROP_TIME_TO_FULL_AVG,
	/* Properties of type `const char *' */
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_SERIAL_NUMBER,
};

/* Power supply Battery device object */
static struct power_supply_desc ft_bat = {

	.name = "phytium-bat",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = ft_bat_props,
	.num_properties = ARRAY_SIZE(ft_bat_props),
	.get_property = ft_bat_get_property,
};
static struct power_supply *ec_bat;

/* Power supply AC property object */
static enum power_supply_property ft_ac_props[] = {

	POWER_SUPPLY_PROP_ONLINE,
};
/* Power supply AC device object */
static struct power_supply_desc ft_ac = {

	.name = "phytium-ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties = ft_ac_props,
	.num_properties = ARRAY_SIZE(ft_ac_props),
	.get_property = ft_ac_get_property,
};
static struct power_supply *ec_ac;



void ft_ec_event_report_entry(struct input_dev *dev, const struct key_entry *ke,
				unsigned int value, bool autorelease)
{
	input_event(dev, EV_MSC, MSC_SCAN, ke->code);
	input_report_key(dev, ke->keycode, value);
	input_sync(dev);
	if (value && autorelease) {
		input_report_key(dev, ke->keycode, 0);
		input_sync(dev);
	}
}

void ft_ec_bl_dn_event(void)
{
	const struct key_entry *key = &ft_keymap[4];

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
}
EXPORT_SYMBOL(ft_ec_bl_dn_event);

void ft_ec_bl_up_event(void)
{
	const struct key_entry *key = &ft_keymap[5];

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
}
EXPORT_SYMBOL(ft_ec_bl_up_event);

void ft_ec_lid_event(void)
{
	const struct key_entry *key = &ft_keymap[0];

	__clear_bit(SW_LID, ft_hotkey_dev->sw);
	input_report_switch(ft_hotkey_dev, key->sw.code, 1);
	input_sync(ft_hotkey_dev);

}
EXPORT_SYMBOL(ft_ec_lid_event);

void ft_ec_display_off(void)
{
	const struct key_entry *key = &ft_keymap[3];

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
	ft_set_display_on(!ft_display_state);
}
EXPORT_SYMBOL(ft_ec_display_off);

void ft_ec_display_event(void)
{
	const struct key_entry *key = &ft_keymap[6];

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
}
EXPORT_SYMBOL(ft_ec_display_event);

void ft_ec_wifi_event(void)
{
	const struct key_entry *key = &ft_keymap[7];

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
}
EXPORT_SYMBOL(ft_ec_wifi_event);

void ft_ec_micmute_event(void)
{
	const struct key_entry *key = &ft_keymap[10];

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
}
EXPORT_SYMBOL(ft_ec_micmute_event);

void ft_ec_airplane_event(void)
{
	const struct key_entry *key = &ft_keymap[11];

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
}

void ft_ec_screen_lock_event(void)
{
	const struct key_entry *key = &ft_keymap[9];

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
}
EXPORT_SYMBOL(ft_ec_screen_lock_event);

void ft_ec_tp_event(void)
{
	const struct key_entry *key = &ft_keymap[2];

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
}
EXPORT_SYMBOL(ft_ec_tp_event);

void ft_ec_screenlock_event(void)
{
	const struct key_entry *key = &ft_keymap[9];

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
}
EXPORT_SYMBOL(ft_ec_screenlock_event);

void ft_sleep_event(void)
{
	const struct key_entry *key = &ft_keymap[1];

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
}
EXPORT_SYMBOL(ft_sleep_event);

void ft_ec_powerbtn_event(void)
{
	const struct key_entry *key = &ft_keymap[8];

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
}
EXPORT_SYMBOL(ft_ec_powerbtn_event);

void ft_ec_screen_rotation_event(void)
{
	int x_val = 0, y_val = 0, z_val = 0;

	x_val = (it8528_read(EC_GSENSOR_X_HIGH) << 8) | it8528_read(EC_GSENSOR_X_LOW);
	y_val = (it8528_read(EC_GSENSOR_Y_HIGH) << 8) | it8528_read(EC_GSENSOR_Y_LOW);
	z_val = (it8528_read(EC_GSENSOR_Z_HIGH) << 8) | it8528_read(EC_GSENSOR_Z_LOW);

	input_event(ec_gsensor_dev, EV_ABS, ABS_X, x_val);
	input_event(ec_gsensor_dev, EV_ABS, ABS_Y, y_val);
	input_event(ec_gsensor_dev, EV_ABS, ABS_Z, z_val);
	input_sync(ec_gsensor_dev);
}

static void ft_ec_powerbtn_event_handler(int event)
{
	ft_ec_powerbtn_event();
}

static void ft_ec_screen_rotation_event_handler(int event)
{
	ft_ec_screen_rotation_event();
}

/* LPC device event handler */
static const struct {
	int event;
	void (*handler)(int event);
} ec_event_table[] = {
	{ EC_EVENT_NUM_LID, ft_lid_handler },
	{ EC_EVENT_NUM_AC, ft_ac_handler },
	{ EC_EVENT_NUM_BAT, ft_bat_handler },
	{ EC_EVENT_NUM_POWERBTN, ft_ec_powerbtn_event_handler },
	{ EC_EVENT_NUM_GSENSOR, ft_ec_screen_rotation_event_handler },
};

void ft_ec_event_handler(int event)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ec_event_table); i++) {
		if (ec_event_table[i].event == event) {
			ec_event_table[i].handler(event);
			return;
		}
	}
}


static irqreturn_t ft_ec_int_routine(int irq, void *dev_id)
{
	int event;
	unsigned long flags;

	spin_lock_irqsave(&i8042_lock, flags);

	/* if not ec irq */
	if (!lpc_ec_interrupt_occurs()) {
		spin_unlock_irqrestore(&i8042_lock, flags);
		return IRQ_NONE;
	}

	/* Clean lpc irq */
	lpc_interrupt_clear_all();

	event = it8528_query_get_event_num();


	if ((event < EC_EVENT_NUM_MIN) || (event > EC_EVENT_NUM_MAX))
		goto exit_event_action;

	ft_ec_event_handler(event);
	spin_unlock_irqrestore(&i8042_lock, flags);
	return IRQ_HANDLED;

exit_event_action:
	spin_unlock_irqrestore(&i8042_lock, flags);
	return IRQ_HANDLED;
}

/* LPC driver init */
static int lpc_int_init(void)
{
	int ret = -EIO;

	pr_debug("phytium: lpc init.\n");

	ft_lpc_device = kmalloc(sizeof(struct lpc_device), GFP_KERNEL);
	if (!ft_lpc_device)
		return -ENOMEM;

	ft_lpc_device->irq = 0x25;
	ft_lpc_device->irq_data = 0x00;
	ft_lpc_device->number = 0x00;
	ft_lpc_device->parameter = 0x00;
	strscpy(ft_lpc_device->name, EC_LPC_DEV, sizeof(ft_lpc_device->name));

	/* get lpc irq */
	ft_lpc_device->irq  = phytium_pio_get_irq();
	if (ft_lpc_device->irq < 0)
		goto out_pdev;


	/* clear all interrupt status */
	lpc_interrupt_clear_all();

	/* Regist pci irq */
	ret = request_irq(ft_lpc_device->irq, ft_ec_int_routine,
				IRQF_SHARED, ft_lpc_device->name, ft_lpc_device);
	if (ret) {
		pr_err("phytium ec: Request irq fail.\n");
		ret = -EFAULT;
		goto out_irq;
	}

	ret = 0;
	pr_debug("phytium: LPC init successful.\n");

	return ret;

out_irq:
out_pdev:
	kfree(ft_lpc_device);
	return ret;
}

/* ec driver interrupt init handler */
static int lpc_driver_init(void)
{
	int ret;

	ret = lpc_int_init();
	if (ret) {
		pr_err("phytium:  Register lpc driver fail.\n");
		return ret;
	}
	pr_debug("phytium: register lpc driver done.\n");
	return ret;
}

int ec_event_int_error;

static void ec_int_routine_func(struct work_struct *work)
{
	int event;
	unsigned long flags;

	spin_lock_irqsave(&i8042_lock, flags);
		event = it8528_query_get_event_num();
		if ((event < EC_EVENT_NUM_MIN) || (event > EC_EVENT_NUM_MAX)) {
			pr_err("ec_int_gpio_routine handle event = 0x%x.\r\n", event);
			ec_event_int_error = true;
			spin_unlock_irqrestore(&i8042_lock, flags);
			return;
		}
		ft_ec_event_handler(event);
	spin_unlock_irqrestore(&i8042_lock, flags);
}
DECLARE_WORK(ec_int_routine_wq, ec_int_routine_func);

static irqreturn_t ft_ec_int_gpio_routine(int irq, void *dev_id)
{
	unsigned long flags;

	spin_lock_irqsave(&i8042_lock, flags);
	disable_irq_nosync(irq);
	if (ec_event_int_error && !it8528_get_ec_evt_flags()) { /*sci_evt flags*/
		goto event_error;
	}

	schedule_work(&ec_int_routine_wq);
event_error:
	enable_irq(irq);
	spin_unlock_irqrestore(&i8042_lock, flags);
	return IRQ_HANDLED;
}

static int set_func_to_gpio(void)
{
	int reg;
	int pin_mul_contrl_base = 0x28180000;
	void __iomem *gpio_func_ioremap = NULL;

	gpio_func_ioremap = ioremap(pin_mul_contrl_base, 0x1000);
	if (!gpio_func_ioremap) {
		pr_err("Could not ioreamp pin_mul_contrl_base!\n");
		return -ENOMEM;
	}

	reg = readl(gpio_func_ioremap + 0x204);

	reg |= BIT(31);
	reg &= ~BIT(30);

	reg &= ~BIT(29);
	reg |= BIT(28);

	writel(reg, gpio_func_ioremap + 0x204);

	iounmap(gpio_func_ioremap);
	return 0;
}

static int gpio_driver_init(int irq)
{
	int ret;

	ft_gpio_device = kmalloc(sizeof(struct lpc_device), GFP_KERNEL);
	if (!ft_gpio_device)
		return -ENOMEM;

	/* Get GPIO7 soft irq*/
	ft_gpio_device->irq = irq;
	strscpy(ft_gpio_device->name, "ec_gpio", sizeof(ft_gpio_device->name));

	/* Configure GPIO pin function */

	ret = set_func_to_gpio();
	if (ret) {
		pr_err("set gpio func failed.\n");
		return -ENOMEM;
	}

	/* Set GPIO related configuration */
	irq_set_irq_type(ft_gpio_device->irq, IRQ_TYPE_EDGE_RISING);
	ret = request_irq(ft_gpio_device->irq, ft_ec_int_gpio_routine,
		  IRQF_TRIGGER_RISING, ft_gpio_device->name, ft_gpio_device);
	if (ret) {
		pr_err("phytium: Request irq fail.\n");
		return -EFAULT;
	}
	return ret;
}

static int ft_set_display_on(bool on)
{
	if (!chassis_types_is_laptop())
		return 0;

	if (on) {
		ft_display_state = true;
		it8528_write(INDEX_DISPLAY_BRIGHTNESS_SET, ft_display_level);
	} else {
		ft_display_state = false;
		ft_display_level = it8528_read(INDEX_DISPLAY_BRIGHTNESS_GET);
		it8528_write(INDEX_DISPLAY_BRIGHTNESS_SET, 0);
	}
	return 0;
}

/* Backlight device set brightness handler */
static int ft_set_brightness(struct backlight_device *pdev)
{
	unsigned int level = 0;

	if (!chassis_types_is_laptop() && !chassis_types_is_allinone())
		return 0;

	if (!ft_display_state)
		return 0;

	level = ((pdev->props.fb_blank == FB_BLANK_UNBLANK) &&
				(pdev->props.power == FB_BLANK_UNBLANK)) ?
					pdev->props.brightness : 0;

	if (level > MAX_BRIGHTNESS)
		level = MAX_BRIGHTNESS;
	else if (level <= 0)
		level = 1;

	it8528_write(INDEX_DISPLAY_BRIGHTNESS_SET, level);

	return 0;
}

/* Backlight device get brightness handler */
static int ft_get_brightness(struct backlight_device *pdev)
{
	if (!chassis_types_is_laptop() && !chassis_types_is_allinone())
		return 0;

	/* Read level from ec */
	return it8528_read(INDEX_DISPLAY_BRIGHTNESS_GET);
}

static void ft_bat_update_temp(void)
{
	int bat_info_value;

	ft_power_info_power_status_update();
	bat_info_value = ec_read16(INDEX_BATTERY_TEMP_HIGH, INDEX_BATTERY_TEMP_LOW);
	power_info->temperature = power_info->bat_in ? (bat_info_value - 2730) : 0;
}

static void ft_bat_update_voltage(void)
{
	int bat_info_value = ec_read16(INDEX_BATTERY_VOL_HIGH, INDEX_BATTERY_VOL_LOW);

	power_info->voltage_now = power_info->bat_in ? bat_info_value : 0;
}

static void ft_bat_update_current(void)
{
	int bat_info_value =
		ec_read16(INDEX_BATTERY_CURRENT_HIGH, INDEX_BATTERY_CURRENT_LOW);

	power_info->current_now = power_info->bat_in ? bat_info_value : 0;
}

static void ft_bat_update_ac(void)
{
	int bat_info_value = ec_read16(INDEX_BATTERY_AC_HIGH, INDEX_BATTERY_AC_LOW);

	ft_power_info_power_status_update();
	power_info->current_average = power_info->bat_in ? bat_info_value : 0;
}

static void ft_bat_update_rc(void)
{
	power_info->remain_capacity =
		ec_read16(INDEX_BATTERY_RC_HIGH, INDEX_BATTERY_RC_LOW);
}

static void ft_bat_update_fcc(void)
{
	power_info->full_charged_capacity =
		ec_read16(INDEX_BATTERY_FCC_HIGH, INDEX_BATTERY_FCC_LOW);
}

static void ft_bat_update_atte(void)
{
	int bat_info_value = ec_read16(INDEX_BATTERY_AC_HIGH, INDEX_BATTERY_AC_LOW);

	bat_info_value = bat_info_value > 0 ? bat_info_value : -bat_info_value;
	if (bat_info_value)
		power_info->remain_time = 60 *
			ec_read16(INDEX_BATTERY_RC_HIGH, INDEX_BATTERY_RC_LOW) /
			bat_info_value;
}

static void ft_bat_update_attf(void)
{
	int bat_info_value =
		ec_read16(INDEX_BATTERY_CAP_HIGH, INDEX_BATTERY_CAP_LOW) -
		ec_read16(INDEX_BATTERY_RC_HIGH, INDEX_BATTERY_RC_LOW);
	int bat_info_value_ac =
		ec_read16(INDEX_BATTERY_AC_HIGH, INDEX_BATTERY_AC_LOW);

	if (bat_info_value_ac > 0)
		power_info->fullchg_time = 60 * bat_info_value / bat_info_value_ac;
}

static void ft_bat_update_rsoc(void)
{
	power_info->remain_capacity_percent = it8528_read(INDEX_BATTERY_CAPACITY);
}

static void ft_bat_update_cyclcnt(void)
{
	power_info->cycle_count =
		ec_read16(INDEX_BATTERY_CYCLECNT_HIGH, INDEX_BATTERY_CYCLECNT_LOW);
}

static void (* const bat_update_table[])(void) = {
	ft_bat_update_temp,
	ft_bat_update_voltage,
	ft_bat_update_current,
	ft_bat_update_ac,
	ft_bat_update_rc,
	ft_bat_update_fcc,
	ft_bat_update_atte,
	ft_bat_update_attf,
	ft_bat_update_rsoc,
	ft_bat_update_cyclcnt,
};

/* Update battery information handle function. */
static void ft_power_battery_info_update(unsigned char bat_reg_flag)
{
	if (bat_reg_flag < BAT_REG_TEMP_FLAG || bat_reg_flag > BAT_REG_CYCLCNT_FLAG)
		return;
	bat_update_table[bat_reg_flag - 1]();
}

/* Update battery information handle function. */
static void ft_power_battery_info_init(void)
{
	int flag;

	pr_debug("%s: start ...\r\n", __func__);


	for (flag = BAT_REG_TEMP_FLAG; flag <= BAT_REG_CYCLCNT_FLAG; flag++)
		ft_power_battery_info_update(flag);

	pr_debug("%s: end ...\r\n", __func__);
}

/* Clear battery static information. */
static void ft_power_info_battery_static_clear(void)
{
	strscpy(power_info->manufacturer_name, "Unknown", sizeof(power_info->manufacturer_name));
	strscpy(power_info->device_name, "Unknown", sizeof(power_info->device_name));
	power_info->technology = POWER_SUPPLY_TECHNOLOGY_UNKNOWN;
	strscpy(power_info->serial_number, "Unknown", sizeof(power_info->serial_number));
	strscpy(power_info->manufacture_date, "Unknown", sizeof(power_info->manufacture_date));
	power_info->cell_count = 0;
	power_info->design_capacity = 0;
	power_info->design_voltage = 0;
}

/* Get battery static information. */
static void ft_power_info_battery_static_update(void)
{
	unsigned int bat_serial_number;

	power_info->technology = POWER_SUPPLY_TECHNOLOGY_LION;

	bat_serial_number = ec_read16(INDEX_BATTERY_SN_HIGH, INDEX_BATTERY_SN_LOW);
	snprintf(power_info->serial_number, 8, "%x", bat_serial_number);

	power_info->cell_count = ec_read16(INDEX_BATTERY_CV_HIGH, INDEX_BATTERY_CV_LOW) / 4200;

	power_info->design_capacity = ec_read16(INDEX_BATTERY_DC_HIGH, INDEX_BATTERY_DC_LOW);
	power_info->design_voltage = ec_read16(INDEX_BATTERY_DV_HIGH, INDEX_BATTERY_DV_LOW);
	power_info->full_charged_capacity =
		ec_read16(INDEX_BATTERY_FCC_HIGH, INDEX_BATTERY_FCC_LOW);
	pr_debug("DesignCapacity: %dmAh, DesignVoltage: %dmV, FullChargeCapacity: %dmAh\n",
		power_info->design_capacity,
		power_info->design_voltage,
		power_info->full_charged_capacity);
}

/* Update power_status value */
static void ft_power_info_power_status_update(void)
{
	unsigned int power_status = 0;

	power_status = it8528_read(INDEX_POWER_STATUS);

	power_info->ac_in = (power_status & MASK(BIT_POWER_ACPRES)) ?
					APM_AC_ONLINE : APM_AC_OFFLINE;

	power_info->bat_in = (power_status & MASK(BIT_POWER_BATPRES)) ? 1 : 0;
	if (power_info->bat_in &&
	    ec_read16(INDEX_BATTERY_DC_HIGH, INDEX_BATTERY_DC_LOW) == 0)
		power_info->bat_in = 0;

	power_info->health = power_info->bat_in ?
				POWER_SUPPLY_HEALTH_GOOD :
				POWER_SUPPLY_HEALTH_UNKNOWN;
	if (!power_info->bat_in) {
		power_info->charge_status = POWER_SUPPLY_STATUS_UNKNOWN;
	} else {
		power_status = it8528_read(INDEX_BATTERY_STATUS);

		if (!(power_status & MASK(BIT_POWER_BATCHG)))
			power_info->charge_status = POWER_SUPPLY_STATUS_CHARGING;
		else if ((power_status & MASK(BIT_POWER_BATFCHG)) &&
	     power_info->ac_in == APM_AC_ONLINE)
			power_info->charge_status = POWER_SUPPLY_STATUS_FULL;
		else
			power_info->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
	}
}

/* Get battery static information string */
static void __maybe_unused ft_bat_get_string(unsigned char index, unsigned char *bat_string)
{
	unsigned char length, i;

	length = it8528_read(index);
	for (i = 0; i < length; i++)
		*bat_string++ = it8528_read(++index);
	*bat_string = '\0';
}

/* Power supply Battery get property handler */
static int ft_bat_get_property(struct power_supply *pws,
			enum power_supply_property psp, union power_supply_propval *val)
{
	unsigned long flags;

	spin_lock_irqsave(&i8042_lock, flags);
	switch (psp) {
		/* Get battery static information. */
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
			val->intval = power_info->design_voltage * 1000; /* mV -> uV */
			break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
			val->intval = power_info->design_capacity * 1000; /* mAh -> uAh */
			break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
			val->strval = power_info->device_name;
			break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
			val->strval = power_info->manufacturer_name;
			break;
	case POWER_SUPPLY_PROP_SERIAL_NUMBER:
			val->strval = power_info->serial_number;
			break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
			val->intval = power_info->technology;
			break;
		/* Get battery dynamic information. */
	case POWER_SUPPLY_PROP_STATUS:
			ft_power_info_power_status_update();
			val->intval = power_info->charge_status;
			break;
	case POWER_SUPPLY_PROP_PRESENT:
			ft_power_info_power_status_update();
			val->intval = power_info->bat_in;
			break;
	case POWER_SUPPLY_PROP_HEALTH:
			ft_power_info_power_status_update();
			val->intval = power_info->health;
			break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
			ft_power_battery_info_update(BAT_REG_CURRENT_FLAG);
			val->intval = power_info->current_now * 1000; /* mA -> uA */
			break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
			ft_power_battery_info_update(BAT_REG_AC_FLAG);
			val->intval = power_info->current_average * 1000; /* mA -> uA */
			break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			ft_power_battery_info_update(BAT_REG_VOLTAGE_FLAG);
			val->intval =  power_info->voltage_now * 1000; /* mV -> uV */
			break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
			ft_power_battery_info_update(BAT_REG_RC_FLAG);
			val->intval = power_info->remain_capacity * 1000; /* mAh -> uAh */
			break;
	case POWER_SUPPLY_PROP_CAPACITY:
			ft_power_battery_info_update(BAT_REG_RSOC_FLAG);
			val->intval = power_info->remain_capacity_percent;	/* Percentage */
			break;
	case POWER_SUPPLY_PROP_TEMP:
			ft_power_battery_info_update(BAT_REG_TEMP_FLAG);
			val->intval = power_info->temperature;	 /* Celcius */
			break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG:
			ft_power_battery_info_update(BAT_REG_ATTE_FLAG);
			if (power_info->remain_time == 0xFFFF)
				power_info->remain_time = 0;
			val->intval = power_info->remain_time * 60;  /* seconds */
			break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_AVG:
			ft_power_battery_info_update(BAT_REG_ATTF_FLAG);
			if (power_info->fullchg_time == 0xFFFF)
				power_info->fullchg_time = 0;
			val->intval = power_info->fullchg_time * 60;  /* seconds */
			break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
			ft_power_battery_info_update(BAT_REG_FCC_FLAG);
			val->intval = power_info->full_charged_capacity * 1000;/* mAh -> uAh */
			break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
			ft_power_battery_info_update(BAT_REG_CYCLCNT_FLAG);
			val->intval = power_info->cycle_count;
			break;
	default:
			spin_unlock_irqrestore(&i8042_lock, flags);
			return -EINVAL;
	}
	spin_unlock_irqrestore(&i8042_lock, flags);
	return 0;
}

/* Power supply AC get property handler */
static int ft_ac_get_property(struct power_supply *pws,
			enum power_supply_property psp, union power_supply_propval *val)
{
	unsigned long flags;

	spin_lock_irqsave(&i8042_lock, flags);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
			ft_power_info_power_status_update();
			val->intval = power_info->ac_in;
			break;
	default:
			spin_unlock_irqrestore(&i8042_lock, flags);
			return -EINVAL;
	}
	spin_unlock_irqrestore(&i8042_lock, flags);
	return 0;
}

/* EC device AC event handler */
static void ft_ac_handler(int status)
{
	/* Report status changed */
	power_supply_changed(ec_ac);
}

/* EC device AC event handler */
int ft_ac_handler_test(void)
{
	/* Report status changed */
	power_supply_changed(ec_ac);

	return 0;
}
EXPORT_SYMBOL(ft_ac_handler_test);

/* LPC device Battery event handler */
static void ft_bat_handler(int status)
{
	/* Battery insert/pull-out to handle battery static information. */
	if (status & MASK(BIT_POWER_BATPRES)) {
		/* If battery is insert, get battery static information. */
		ft_power_info_battery_static_update();
	} else {
		/* Else if battery is pull-out, clear battery static information. */
		ft_power_info_battery_static_clear();
	}
	/* Report status changed */
	power_supply_changed(ec_bat);
}

/* LPC device LID event handler */
static void ft_lid_handler(int status)
{
	ft_ec_lid_event();
}

/* Hotkey device init */
int ft_hotkey_init(void)
{
	int ret;

	if (ft_hotkey_dev)
		return 0;

	ft_hotkey_dev = input_allocate_device();
	if (!ft_hotkey_dev)
		return -ENOMEM;

	ft_hotkey_dev->name = "phytium Laptop Hotkeys";
	ft_hotkey_dev->phys = "button/input0";
	ft_hotkey_dev->id.bustype = BUS_HOST;
	ft_hotkey_dev->dev.parent = NULL;
	set_bit(EV_KEY, ft_hotkey_dev->evbit);
	set_bit(EV_SW, ft_hotkey_dev->evbit);
	set_bit(KEY_POWER, ft_hotkey_dev->keybit);
	set_bit(SW_LID, ft_hotkey_dev->swbit);


	ret = sparse_keymap_setup(ft_hotkey_dev, ft_keymap, NULL);
	if (ret) {
		pr_err("phytium Laptop Platform Driver: Fail to setup input device keymap\n");
		input_free_device(ft_hotkey_dev);

		return ret;
	}

	ret = input_register_device(ft_hotkey_dev);
	if (ret) {
		input_free_device(ft_hotkey_dev);

		return ret;
	}
	return 0;
}
EXPORT_SYMBOL(ft_hotkey_init);

/* Hotkey device exit */
static void ft_hotkey_exit(void)
{
	if (ft_hotkey_dev) {
		input_unregister_device(ft_hotkey_dev);
		ft_hotkey_dev = NULL;
	}
}

static ssize_t gsensor_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int x_val = 0;

	x_val = (it8528_read(EC_GSENSOR_X_HIGH) << 8) | it8528_read(EC_GSENSOR_X_LOW);
	return sprintf(buf, "%d\n", x_val);
}

static struct kobj_attribute gsensor_attr =
	__ATTR(value_x, 0644, gsensor_show, NULL);

static struct attribute *gsensor_attrs[] = {
	&gsensor_attr.attr,
	NULL,
};

static struct attribute_group gsensor_attr_group = {
	.attrs = gsensor_attrs,
};

static struct kobject *gsensor_kobj;

/* Gsensor device init */
int ec_gsonsor_init(struct platform_device *pdev)
{
	int ret;

	if (ec_gsensor_dev)
		return 0;

	ec_gsensor_dev = input_allocate_device();
	if (!ec_gsensor_dev)
		return -ENOMEM;

	ec_gsensor_dev->name = "EC_Gsensor";
	ec_gsensor_dev->uniq = "EC_Gsensor";
	ec_gsensor_dev->id.bustype = BUS_HOST;
	ec_gsensor_dev->dev.parent = &pdev->dev;

	set_bit(EV_SYN, ec_gsensor_dev->evbit);
	set_bit(EV_ABS, ec_gsensor_dev->evbit);
	set_bit(ABS_X, ec_gsensor_dev->absbit);
	set_bit(ABS_Y, ec_gsensor_dev->absbit);
	set_bit(ABS_Z, ec_gsensor_dev->absbit);

	input_set_capability(ec_gsensor_dev, EV_ABS, ABS_X);
	input_set_capability(ec_gsensor_dev, EV_ABS, ABS_Y);
	input_set_capability(ec_gsensor_dev, EV_ABS, ABS_Z);
	input_set_abs_params(ec_gsensor_dev, ABS_X, 0, 0xFFFF, 0, 0);
	input_set_abs_params(ec_gsensor_dev, ABS_Y, 0, 0xFFFF, 0, 0);
	input_set_abs_params(ec_gsensor_dev, ABS_Z, 0, 0xFFFF, 0, 0);

	ret = input_register_device(ec_gsensor_dev);
	if (ret) {
		input_free_device(ec_gsensor_dev);
		return ret;
	}

	gsensor_kobj = kobject_create_and_add("gsensor", &pdev->dev.kobj);
	if (!gsensor_kobj)
		return -ENOMEM;
	ret = sysfs_create_group(gsensor_kobj, &gsensor_attr_group);
	if (ret) {
		kobject_put(gsensor_kobj);
		return ret;
	}

	return 0;
}

/* Gsensor device exit */
static void ec_gsensor_exit(void)
{
	if (ec_gsensor_dev) {
		input_unregister_device(ec_gsensor_dev);
		ec_gsensor_dev = NULL;
	}
}

#ifdef CONFIG_PM

static int ft_laptop_suspend(struct device *dev)
{
	if (ft_gpio_device)
		disable_irq_nosync(ft_gpio_device->irq);
	it8528_ec_event_int_disable();
	return 0;
}

static int ft_laptop_resume(struct device *dev)
{
	int ret;

	if (ft_gpio_device) {
		/* Configure GPIO pin function */
		ret = set_func_to_gpio();
		if (ret) {
			pr_err("set gpio func failed.\n");
			return -ENOMEM;
		}
		/*Set GPIO related configuration*/
		irq_set_irq_type(ft_gpio_device->irq, IRQ_TYPE_EDGE_RISING);
		it8528_ec_event_int_enable();
		/* clean ec evnet queue*/
		ret = it8528_query_clean_event();
		if (ret) {
			pr_err("EC clean event fail.\n");
			return -EINVAL;
		}
	}

	if (ft_gpio_device)
		enable_irq(ft_gpio_device->irq);
	/* enable ec event interrupt */
	it8528_ec_event_int_enable();

	if (chassis_types_is_laptop()) { /* chassis is laptop */
		ft_power_info_power_status_update();
		if (power_info->bat_in) {
			/* Get battery static information. */
			ft_power_info_battery_static_update();

			/* Report status changed */
			power_supply_changed(ec_ac);
		}

		input_report_switch(ft_hotkey_dev, SW_LID, 0);
		input_sync(ft_hotkey_dev);
	}

	return 0;
}
#endif


static int ft_lpc_event_probe(struct platform_device *dev)
{
	int ret = 0;
	int gpio_irq;
	int brightness_default;

	pr_debug("phytium ec: in probe!\n");

	if (read_cpuid_implementor() != ARM_CPU_IMP_PHYTIUM)
		return ret;

	ret = it8528_init();
	if (ret) {
		pr_info("laptop_init: ec chip not found!\n");
		return ret;
	}

	/* Regist backlight */
	if (chassis_types_is_laptop() || chassis_types_is_allinone()) {
		ft_backlight_dev = devm_backlight_device_register(&dev->dev, "phytium",
				&dev->dev, NULL, &ft_backlight_ops, NULL);
		if (IS_ERR(ft_backlight_dev)) {
			ret = PTR_ERR(ft_backlight_dev);
			return ret;
		}
		ft_backlight_dev->props.max_brightness = 100;

		brightness_default = it8528_read(INDEX_DISPLAY_BRIGHTNESS_GET);
		if (brightness_default == 0) {
			ft_backlight_dev->props.brightness = ft_backlight_dev->props.max_brightness;
			it8528_write(INDEX_DISPLAY_BRIGHTNESS_SET,
	     ft_backlight_dev->props.max_brightness);
		} else {
			ft_backlight_dev->props.brightness = brightness_default;
		}

		backlight_update_status(ft_backlight_dev);
	}

	ret = ft_hotkey_init();
	if (ret) {
		pr_err("phytium Platform Driver: Hotkey init fail.\n");
		return ret;
	}

	ret = ec_gsonsor_init(dev);
	if (ret) {
		pr_err("phytium Platform Driver: Gsensor init fail.\n");
		ft_hotkey_exit();
		return ret;
	}

	/* Register power supply START */
	power_info = devm_kzalloc(&dev->dev, sizeof(struct ft_power_info), GFP_KERNEL);
	if (!power_info) {
		ret = -ENOMEM;
		goto fail_after_gsensor;
	}

	ft_power_battery_info_init();

	ft_power_info_power_status_update();
	if (power_info->bat_in) {
		/* Get battery static information. */
		ft_power_info_battery_static_update();
	} else {
		pr_info("phytium Platform Driver: The battery does not exist!!\n");
	}

	if (chassis_types_is_laptop() && power_info->bat_in) {
		ec_bat = devm_power_supply_register(&dev->dev, &ft_bat, NULL);
		if (IS_ERR(ec_bat)) {
			ret = -ENOMEM;
			goto fail_after_gsensor;
		}

		ec_ac = devm_power_supply_register(&dev->dev, &ft_ac, NULL);
		if (IS_ERR(ec_ac)) {
			ret = -ENOMEM;
			goto fail_after_gsensor;
		}
	}
	/* Register power supply END */

	/* Register Temp and Fan hwmon */
	ec_sensors_register(dev);

	gpio_irq = acpi_dev_gpio_irq_get(ACPI_COMPANION(&(dev->dev)), 0);
	if (gpio_irq > 0) {
		/* GPIO Driver Init Start */
		ret = gpio_driver_init(gpio_irq);
		if (ret) {
			pr_err("phytium ec: gpio driver init fail.\n");
			goto fail_after_gsensor;
		}
	} else {
		/* LPC Driver Init Start */
		ret = lpc_driver_init();
		if (ret) {
			pr_err("phytium: LPC driver init fail.\n");
			goto fail_after_gsensor;
		}
	}

	/* clean ec evnet queue*/
	ret = it8528_query_clean_event();
	if (ret) {
		pr_err("EC clean event fail.\n");
		goto fail_after_gsensor;
	}

	/* enable ec event interrupt */
	it8528_ec_event_int_enable();

	pr_debug("phytium: ec event driver init done...\n");

	return 0;

fail_after_gsensor:
	ec_gsensor_exit();
	ft_hotkey_exit();
	return ret;
}

module_platform_driver(ft_ec_pdriver);

MODULE_DESCRIPTION("phytium Laptop Driver");
MODULE_LICENSE("GPL");
