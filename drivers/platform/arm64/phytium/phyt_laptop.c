// SPDX-License-Identifier: GPL-2.0
#include <linux/io.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/power_supply.h>
#include <linux/input.h>
#include <linux/input/sparse-keymap.h>
#include <linux/module.h>
#include <linux/string.h>
#include "ec_it8528.h"
#include <linux/acpi.h>
#include <linux/cputypes.h>
#include <linux/classtypes.h>
#include <linux/gpio.h>

static int ec_read16(u8 high_index, u8 low_index, u16 *value)
{
	u8 high, low;
	int ret;

	ret = it8528_read_byte(high_index, &high);
	if (ret)
		return ret;

	ret = it8528_read_byte(low_index, &low);
	if (ret)
		return ret;

	*value = (high << 8) | low;
	return 0;
}


static int ec_gsensor_init(struct platform_device *pdev);

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
	/* Irq relative */
	unsigned int irq;
	/* Device name */
	unsigned char name[10];
};


/* LPC device object */
static struct lpc_device *ft_lpc_device;
static struct lpc_device *ft_gpio_device;
static int ft_lpc_event_probe(struct platform_device *);
static int ft_lpc_event_remove(struct platform_device *);

static int ft_get_brightness(struct backlight_device *);
static int ft_set_brightness(struct backlight_device *);

/* >>>Power management operation */
/* Update battery information handle function. */
static int ft_power_battery_info_update(unsigned char bat_reg_flag);
/* Clear battery static information. */
static void ft_power_info_battery_static_clear(void);
/* Get battery static information. */
static int ft_power_info_battery_static_update(void);
/* Update power_status value */
static int ft_power_info_power_status_update(void);
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
	.remove = ft_lpc_event_remove,
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

static void ft_ec_powerbtn_event(void)
{
	const struct key_entry *key = &ft_keymap[8];

	if (!ft_hotkey_dev)
		return;

	sparse_keymap_report_entry(ft_hotkey_dev, key, 1, true);
}

static void ft_ec_screen_rotation_event(void)
{
	u16 x_val, y_val, z_val;

	if (!ec_gsensor_dev)
		return;

	if (ec_read16(EC_GSENSOR_X_HIGH, EC_GSENSOR_X_LOW, &x_val) ||
	    ec_read16(EC_GSENSOR_Y_HIGH, EC_GSENSOR_Y_LOW, &y_val) ||
	    ec_read16(EC_GSENSOR_Z_HIGH, EC_GSENSOR_Z_LOW, &z_val))
		return;

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

/* Whether the EC event number has a consumer, special or keymap. */
static bool ec_event_is_known(unsigned int event)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ec_event_table); i++) {
		if (ec_event_table[i].event == event)
			return true;
	}

	if (!ft_hotkey_dev)
		return false;

	return sparse_keymap_entry_from_scancode(ft_hotkey_dev, event) != NULL;
}

static void ft_ec_event_handler(int event)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ec_event_table); i++) {
		if (ec_event_table[i].event == event) {
			ec_event_table[i].handler(event);
			return;
		}
	}

	/* Everything else in the keymap is a plain key press. */
	sparse_keymap_report_event(ft_hotkey_dev, event, 1, true);
}

/* Upper bound on how many queued EC events one interrupt drains. */
#define EC_EVENT_DRAIN_MAX	16

/*
 * Consume every event the EC has queued. Querying an event also removes it
 * from the EC, so a single notification can stand for several events.
 */
static void ec_drain_events(void)
{
	int i;

	for (i = 0; i < EC_EVENT_DRAIN_MAX; i++) {
		int event;

		if (!it8528_get_ec_evt_flags())
			break;

		event = it8528_query_get_event_num();
		if (event < 0) {
			pr_err("phytium ec: failed to read event: %d\n", event);
			break;
		}

		if (event > 0 && ec_event_is_known(event))
			ft_ec_event_handler(event);
	}
}

/* EC event interrupt, shared with i8042 over the PIO controller. */
static irqreturn_t ft_ec_int_routine(int irq, void *dev_id)
{
	unsigned long flags;

	/*
	 * Test the EC status port rather than the PIO latch: i8042 may have
	 * acknowledged the shared interrupt before us and cleared the latch
	 * while the EC still has an event queued.
	 */
	if (!it8528_get_ec_evt_flags())
		return IRQ_NONE;

	spin_lock_irqsave(&i8042_lock, flags);
	lpc_ec_interrupt_clear();
	ec_drain_events();
	spin_unlock_irqrestore(&i8042_lock, flags);

	return IRQ_HANDLED;
}

/* LPC driver init */
static int lpc_int_init(void)
{
	int ret;

	pr_debug("phytium: lpc init.\n");

	ft_lpc_device = kmalloc(sizeof(struct lpc_device), GFP_KERNEL);
	if (!ft_lpc_device)
		return -ENOMEM;

	strscpy(ft_lpc_device->name, EC_LPC_DEV, sizeof(ft_lpc_device->name));

	/* get lpc irq */
	ret = phytium_pio_get_irq();
	if (ret < 0) {
		pr_err("phytium ec: Failed to get lpc irq: %d\n", ret);
		goto err_free;
	}
	ft_lpc_device->irq = ret;

	/* Clear the stale EC source without disturbing i8042's sources. */
	lpc_ec_interrupt_clear();

	/* Regist pci irq */
	ret = request_irq(ft_lpc_device->irq, ft_ec_int_routine,
				IRQF_SHARED, ft_lpc_device->name, ft_lpc_device);
	if (ret) {
		pr_err("phytium ec: Request irq fail: %d\n", ret);
		goto err_free;
	}

	pr_debug("phytium: LPC init successful.\n");

	return 0;

err_free:
	kfree(ft_lpc_device);
	ft_lpc_device = NULL;
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

/* GPIO interrupt bottom half: drain the queue, then rearm the edge. */
static void ec_int_routine_func(struct work_struct *work)
{
	unsigned long flags;

	spin_lock_irqsave(&i8042_lock, flags);
	ec_drain_events();
	spin_unlock_irqrestore(&i8042_lock, flags);

	/*
	 * The GPIO interrupt is edge triggered and will not fire again for
	 * events drained above, so only rearm once the queue is empty.
	 */
	if (ft_gpio_device)
		enable_irq(ft_gpio_device->irq);
}
DECLARE_WORK(ec_int_routine_wq, ec_int_routine_func);

static irqreturn_t ft_ec_int_gpio_routine(int irq, void *dev_id)
{
	disable_irq_nosync(irq);
	schedule_work(&ec_int_routine_wq);

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
		pr_err("set gpio func failed: %d\n", ret);
		goto err_free;
	}

	/* Set GPIO related configuration */
	irq_set_irq_type(ft_gpio_device->irq, IRQ_TYPE_EDGE_RISING);
	ret = request_irq(ft_gpio_device->irq, ft_ec_int_gpio_routine,
		  IRQF_TRIGGER_RISING, ft_gpio_device->name, ft_gpio_device);
	if (ret) {
		pr_err("phytium: Request irq fail: %d\n", ret);
		goto err_free;
	}
	return 0;

err_free:
	kfree(ft_gpio_device);
	ft_gpio_device = NULL;
	return ret;
}

/* Backlight device set brightness handler */
static int ft_set_brightness(struct backlight_device *pdev)
{
	unsigned int level = 0;

	if (!chassis_types_is_laptop() && !chassis_types_is_allinone())
		return 0;

	level = ((pdev->props.fb_blank == FB_BLANK_UNBLANK) &&
				(pdev->props.power == FB_BLANK_UNBLANK)) ?
					pdev->props.brightness : 0;

	if (level > MAX_BRIGHTNESS)
		level = MAX_BRIGHTNESS;

	/* Zero is the EC's backlight-off level, so it must be kept as is. */
	return it8528_write(INDEX_DISPLAY_BRIGHTNESS_SET, level);
}

/* Backlight device get brightness handler */
static int ft_get_brightness(struct backlight_device *pdev)
{
	if (!chassis_types_is_laptop() && !chassis_types_is_allinone())
		return 0;

	u8 level;
	int ret;

	/* Read level from ec */
	ret = it8528_read_byte(INDEX_DISPLAY_BRIGHTNESS_GET, &level);
	if (ret)
		return ret;

	return level;
}

static int ft_bat_update_temp(void)
{
	u16 value;
	int ret;

	ret = ft_power_info_power_status_update();
	if (ret)
		return ret;

	ret = ec_read16(INDEX_BATTERY_TEMP_HIGH, INDEX_BATTERY_TEMP_LOW, &value);
	if (ret)
		return ret;

	power_info->temperature = power_info->bat_in ? ((int)value - 2730) : 0;
	return 0;
}

static int ft_bat_update_voltage(void)
{
	u16 value;
	int ret;

	ret = ec_read16(INDEX_BATTERY_VOL_HIGH, INDEX_BATTERY_VOL_LOW, &value);
	if (ret)
		return ret;

	power_info->voltage_now = power_info->bat_in ? value : 0;
	return 0;
}

static int ft_bat_update_current(void)
{
	u16 value;
	int ret;

	ret = ec_read16(INDEX_BATTERY_CURRENT_HIGH, INDEX_BATTERY_CURRENT_LOW,
			&value);
	if (ret)
		return ret;

	/* Battery current is signed: negative while discharging. */
	power_info->current_now = power_info->bat_in ? (s16)value : 0;
	return 0;
}

static int ft_bat_update_ac(void)
{
	u16 value;
	int ret;

	ret = ft_power_info_power_status_update();
	if (ret)
		return ret;

	ret = ec_read16(INDEX_BATTERY_AC_HIGH, INDEX_BATTERY_AC_LOW, &value);
	if (ret)
		return ret;

	power_info->current_average = power_info->bat_in ? (s16)value : 0;
	return 0;
}

static int ft_bat_update_rc(void)
{
	u16 value;
	int ret;

	ret = ec_read16(INDEX_BATTERY_RC_HIGH, INDEX_BATTERY_RC_LOW, &value);
	if (ret)
		return ret;

	power_info->remain_capacity = value;
	return 0;
}

static int ft_bat_update_fcc(void)
{
	u16 value;
	int ret;

	ret = ec_read16(INDEX_BATTERY_FCC_HIGH, INDEX_BATTERY_FCC_LOW, &value);
	if (ret)
		return ret;

	power_info->full_charged_capacity = value;
	return 0;
}

static int ft_bat_update_atte(void)
{
	u16 rc, ac_raw;
	int current_ma;
	int ret;

	ret = ec_read16(INDEX_BATTERY_AC_HIGH, INDEX_BATTERY_AC_LOW, &ac_raw);
	if (ret)
		return ret;

	ret = ec_read16(INDEX_BATTERY_RC_HIGH, INDEX_BATTERY_RC_LOW, &rc);
	if (ret)
		return ret;

	current_ma = (s16)ac_raw;
	if (current_ma < 0)
		current_ma = -current_ma;

	power_info->remain_time = current_ma ? 60 * rc / current_ma : 0;
	return 0;
}

static int ft_bat_update_attf(void)
{
	u16 cap, rc, ac_raw;
	int current_ma, remaining;
	int ret;

	ret = ec_read16(INDEX_BATTERY_CAP_HIGH, INDEX_BATTERY_CAP_LOW, &cap);
	if (ret)
		return ret;

	ret = ec_read16(INDEX_BATTERY_RC_HIGH, INDEX_BATTERY_RC_LOW, &rc);
	if (ret)
		return ret;

	ret = ec_read16(INDEX_BATTERY_AC_HIGH, INDEX_BATTERY_AC_LOW, &ac_raw);
	if (ret)
		return ret;

	current_ma = (s16)ac_raw;
	if (current_ma <= 0) {
		/* Not charging, so there is no meaningful time to full. */
		power_info->fullchg_time = 0;
		return 0;
	}

	remaining = (int)cap - (int)rc;
	if (remaining < 0)
		remaining = 0;

	power_info->fullchg_time = 60 * remaining / current_ma;
	return 0;
}

static int ft_bat_update_rsoc(void)
{
	u8 value;
	int ret;

	ret = it8528_read_byte(INDEX_BATTERY_CAPACITY, &value);
	if (ret)
		return ret;

	power_info->remain_capacity_percent = value;
	return 0;
}

static int ft_bat_update_cyclcnt(void)
{
	u16 value;
	int ret;

	ret = ec_read16(INDEX_BATTERY_CYCLECNT_HIGH,
			INDEX_BATTERY_CYCLECNT_LOW, &value);
	if (ret)
		return ret;

	power_info->cycle_count = value;
	return 0;
}

static int (* const bat_update_table[])(void) = {
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
static int ft_power_battery_info_update(unsigned char bat_reg_flag)
{
	if (bat_reg_flag < BAT_REG_TEMP_FLAG || bat_reg_flag > BAT_REG_CYCLCNT_FLAG)
		return -EINVAL;

	return bat_update_table[bat_reg_flag - 1]();
}

/* Read every cached battery property, stopping at the first EC failure. */
static int ft_power_battery_info_init(void)
{
	int flag, ret;

	pr_debug("%s: start ...\r\n", __func__);

	for (flag = BAT_REG_TEMP_FLAG; flag <= BAT_REG_CYCLCNT_FLAG; flag++) {
		ret = ft_power_battery_info_update(flag);
		if (ret)
			return ret;
	}

	pr_debug("%s: end ...\r\n", __func__);
	return 0;
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
static int ft_power_info_battery_static_update(void)
{
	u16 serial_number, cell_voltage, design_capacity;
	u16 design_voltage, full_charged_capacity;
	int ret;

	ret = ec_read16(INDEX_BATTERY_SN_HIGH, INDEX_BATTERY_SN_LOW, &serial_number);
	if (ret)
		return ret;

	ret = ec_read16(INDEX_BATTERY_CV_HIGH, INDEX_BATTERY_CV_LOW, &cell_voltage);
	if (ret)
		return ret;

	ret = ec_read16(INDEX_BATTERY_DC_HIGH, INDEX_BATTERY_DC_LOW, &design_capacity);
	if (ret)
		return ret;

	ret = ec_read16(INDEX_BATTERY_DV_HIGH, INDEX_BATTERY_DV_LOW, &design_voltage);
	if (ret)
		return ret;

	ret = ec_read16(INDEX_BATTERY_FCC_HIGH, INDEX_BATTERY_FCC_LOW,
			&full_charged_capacity);
	if (ret)
		return ret;

	power_info->technology = POWER_SUPPLY_TECHNOLOGY_LION;
	snprintf(power_info->serial_number, sizeof(power_info->serial_number),
		 "%x", serial_number);
	power_info->cell_count = cell_voltage / 4200;
	power_info->design_capacity = design_capacity;
	power_info->design_voltage = design_voltage;
	power_info->full_charged_capacity = full_charged_capacity;

	pr_debug("DesignCapacity: %dmAh, DesignVoltage: %dmV, FullChargeCapacity: %dmAh\n",
		power_info->design_capacity,
		power_info->design_voltage,
		power_info->full_charged_capacity);
	return 0;
}

/* Refresh AC/battery presence and charge status from the EC. */
static int ft_power_info_power_status_update(void)
{
	u16 design_capacity;
	u8 status;
	int ret;

	ret = it8528_read_byte(INDEX_POWER_STATUS, &status);
	if (ret)
		return ret;

	power_info->ac_in = (status & MASK(BIT_POWER_ACPRES)) ?
					APM_AC_ONLINE : APM_AC_OFFLINE;

	power_info->bat_in = !!(status & MASK(BIT_POWER_BATPRES));
	if (power_info->bat_in) {
		ret = ec_read16(INDEX_BATTERY_DC_HIGH, INDEX_BATTERY_DC_LOW,
				&design_capacity);
		if (ret)
			return ret;

		/* A pack that reports no design capacity is not usable. */
		if (!design_capacity)
			power_info->bat_in = 0;
	}

	power_info->health = power_info->bat_in ?
				POWER_SUPPLY_HEALTH_GOOD :
				POWER_SUPPLY_HEALTH_UNKNOWN;
	if (!power_info->bat_in) {
		power_info->charge_status = POWER_SUPPLY_STATUS_UNKNOWN;
		return 0;
	}

	ret = it8528_read_byte(INDEX_BATTERY_STATUS, &status);
	if (ret)
		return ret;

	if (!(status & MASK(BIT_POWER_BATCHG)))
		power_info->charge_status = POWER_SUPPLY_STATUS_CHARGING;
	else if ((status & MASK(BIT_POWER_BATFCHG)) &&
		 power_info->ac_in == APM_AC_ONLINE)
		power_info->charge_status = POWER_SUPPLY_STATUS_FULL;
	else
		power_info->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;

	return 0;
}

/* Power supply Battery get property handler */
static int ft_bat_get_property(struct power_supply *pws,
			enum power_supply_property psp, union power_supply_propval *val)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&i8042_lock, flags);
	switch (psp) {
		/* Cached battery static information. */
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
		/* Battery dynamic information, read back from the EC. */
	case POWER_SUPPLY_PROP_STATUS:
			ret = ft_power_info_power_status_update();
			if (!ret)
				val->intval = power_info->charge_status;
			break;
	case POWER_SUPPLY_PROP_PRESENT:
			ret = ft_power_info_power_status_update();
			if (!ret)
				val->intval = power_info->bat_in;
			break;
	case POWER_SUPPLY_PROP_HEALTH:
			ret = ft_power_info_power_status_update();
			if (!ret)
				val->intval = power_info->health;
			break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
			ret = ft_power_battery_info_update(BAT_REG_CURRENT_FLAG);
			if (!ret)
				val->intval = power_info->current_now * 1000; /* mA -> uA */
			break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
			ret = ft_power_battery_info_update(BAT_REG_AC_FLAG);
			if (!ret)
				val->intval = power_info->current_average * 1000;
			break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			ret = ft_power_battery_info_update(BAT_REG_VOLTAGE_FLAG);
			if (!ret)
				val->intval = power_info->voltage_now * 1000; /* mV -> uV */
			break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
			ret = ft_power_battery_info_update(BAT_REG_RC_FLAG);
			if (!ret)
				val->intval = power_info->remain_capacity * 1000;
			break;
	case POWER_SUPPLY_PROP_CAPACITY:
			ret = ft_power_battery_info_update(BAT_REG_RSOC_FLAG);
			if (!ret)
				val->intval = power_info->remain_capacity_percent;
			break;
	case POWER_SUPPLY_PROP_TEMP:
			ret = ft_power_battery_info_update(BAT_REG_TEMP_FLAG);
			if (!ret)
				val->intval = power_info->temperature; /* Celsius */
			break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG:
			ret = ft_power_battery_info_update(BAT_REG_ATTE_FLAG);
			if (!ret) {
				if (power_info->remain_time == 0xFFFF)
					power_info->remain_time = 0;
				val->intval = power_info->remain_time * 60; /* seconds */
			}
			break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_AVG:
			ret = ft_power_battery_info_update(BAT_REG_ATTF_FLAG);
			if (!ret) {
				if (power_info->fullchg_time == 0xFFFF)
					power_info->fullchg_time = 0;
				val->intval = power_info->fullchg_time * 60; /* seconds */
			}
			break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
			ret = ft_power_battery_info_update(BAT_REG_FCC_FLAG);
			if (!ret)
				val->intval = power_info->full_charged_capacity * 1000;
			break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
			ret = ft_power_battery_info_update(BAT_REG_CYCLCNT_FLAG);
			if (!ret)
				val->intval = power_info->cycle_count;
			break;
	default:
			ret = -EINVAL;
			break;
	}
	spin_unlock_irqrestore(&i8042_lock, flags);

	return ret;
}

/* Power supply AC get property handler */
static int ft_ac_get_property(struct power_supply *pws,
			enum power_supply_property psp, union power_supply_propval *val)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&i8042_lock, flags);
	ret = ft_power_info_power_status_update();
	if (!ret)
		val->intval = power_info->ac_in;
	spin_unlock_irqrestore(&i8042_lock, flags);

	return ret;
}

/* EC device AC event handler */
static void ft_ac_handler(int status)
{
	if (ec_ac)
		power_supply_changed(ec_ac);
}

/*
 * Report the switch state the EC currently holds. The input core keeps its
 * own view of the switch, so its bitmap must not be edited behind its back.
 */
static void ft_lid_report(void)
{
	u8 status;
	int ret;

	if (!ft_hotkey_dev)
		return;

	ret = it8528_read_byte(INDEX_LID_STATUS, &status);
	if (ret) {
		pr_err("phytium ec: failed to read lid status: %d\n", ret);
		return;
	}

	/* LID_STATUS_BIT is set while the lid is closed. */
	input_report_switch(ft_hotkey_dev, SW_LID, !(status & LID_STATUS_BIT));
	input_sync(ft_hotkey_dev);
}

/* LPC device Battery event handler */
static void ft_bat_handler(int status)
{
	/* status carries the event number, not the power status register. */
	if (ft_power_info_power_status_update())
		return;

	/* Battery insert/pull-out to handle battery static information. */
	if (power_info->bat_in) {
		/* If battery is insert, get battery static information. */
		ft_power_info_battery_static_update();
	} else {
		/* Else if battery is pull-out, clear battery static information. */
		ft_power_info_battery_static_clear();
	}

	if (ec_bat)
		power_supply_changed(ec_bat);
}

/* LPC device LID event handler */
static void ft_lid_handler(int status)
{
	ft_lid_report();
}

/* Hotkey device init */
static int ft_hotkey_init(void)
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
		ft_hotkey_dev = NULL;

		return ret;
	}

	ret = input_register_device(ft_hotkey_dev);
	if (ret) {
		input_free_device(ft_hotkey_dev);
		ft_hotkey_dev = NULL;

		return ret;
	}
	return 0;
}

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
	u16 x_val;
	int ret;

	ret = ec_read16(EC_GSENSOR_X_HIGH, EC_GSENSOR_X_LOW, &x_val);
	if (ret)
		return ret;

	return sysfs_emit(buf, "%d\n", x_val);
}

static struct kobj_attribute gsensor_attr =
	__ATTR(value_x, 0444, gsensor_show, NULL);

static struct attribute *gsensor_attrs[] = {
	&gsensor_attr.attr,
	NULL,
};

static struct attribute_group gsensor_attr_group = {
	.attrs = gsensor_attrs,
};

static struct kobject *gsensor_kobj;

/* Gsensor device init */
static int ec_gsensor_init(struct platform_device *pdev)
{
	struct kobject *kobj;
	int ret;

	if (ec_gsensor_dev)
		return 0;

	if (!gsensor_kobj) {
		kobj = kobject_create_and_add("gsensor", &pdev->dev.kobj);
		if (!kobj)
			return -ENOMEM;

		ret = sysfs_create_group(kobj, &gsensor_attr_group);
		if (ret) {
			kobject_put(kobj);
			return ret;
		}

		/* Only publish the kobject once it is fully set up. */
		gsensor_kobj = kobj;
	}

	ec_gsensor_dev = input_allocate_device();
	if (!ec_gsensor_dev) {
		ret = -ENOMEM;
		goto err_sysfs;
	}

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
	if (ret)
		goto err_free_input;

	return 0;

err_free_input:
	/* input_allocate_device() memory is freed here, so drop the pointer. */
	input_free_device(ec_gsensor_dev);
	ec_gsensor_dev = NULL;
err_sysfs:
	if (gsensor_kobj) {
		sysfs_remove_group(gsensor_kobj, &gsensor_attr_group);
		kobject_put(gsensor_kobj);
		gsensor_kobj = NULL;
	}
	return ret;
}

/* Gsensor device exit */
static void ec_gsensor_exit(void)
{
	if (gsensor_kobj) {
		sysfs_remove_group(gsensor_kobj, &gsensor_attr_group);
		kobject_put(gsensor_kobj);
		gsensor_kobj = NULL;
	}

	if (ec_gsensor_dev) {
		input_unregister_device(ec_gsensor_dev);
		ec_gsensor_dev = NULL;
	}
}

/*
 * Tear down the EC event interrupt. Must run before the structures the
 * handlers reach through, since the work item may still be queued when
 * free_irq() returns.
 */
static void ft_ec_irq_exit(void)
{
	it8528_ec_event_int_disable();

	if (ft_gpio_device)
		free_irq(ft_gpio_device->irq, ft_gpio_device);

	if (ft_lpc_device)
		free_irq(ft_lpc_device->irq, ft_lpc_device);

	/* The worker rearms ft_gpio_device, so drain it before freeing. */
	cancel_work_sync(&ec_int_routine_wq);

	kfree(ft_gpio_device);
	ft_gpio_device = NULL;
	kfree(ft_lpc_device);
	ft_lpc_device = NULL;
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
			pr_err("set gpio func failed: %d\n", ret);
			return ret;
		}
		/*Set GPIO related configuration*/
		irq_set_irq_type(ft_gpio_device->irq, IRQ_TYPE_EDGE_RISING);
		/* clean ec evnet queue*/
		ret = it8528_query_clean_event();
		if (ret) {
			pr_err("EC clean event fail: %d\n", ret);
			return ret;
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
			if (ec_ac)
				power_supply_changed(ec_ac);
		}

		/* A laptop can resume with the lid still closed. */
		ft_lid_report();
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
		return -ENODEV;

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

		brightness_default = ft_get_brightness(ft_backlight_dev);
		if (brightness_default < 0) {
			pr_err("phytium ec: failed to read brightness: %d\n",
			       brightness_default);
			return brightness_default;
		}

		if (brightness_default == 0)
			ft_backlight_dev->props.brightness =
				ft_backlight_dev->props.max_brightness;
		else
			ft_backlight_dev->props.brightness = brightness_default;

		backlight_update_status(ft_backlight_dev);
	}

	ret = ft_hotkey_init();
	if (ret) {
		pr_err("phytium Platform Driver: Hotkey init fail.\n");
		return ret;
	}

	ret = ec_gsensor_init(dev);
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

	ret = ft_power_battery_info_init();
	if (ret) {
		pr_err("phytium ec: failed to read battery information: %d\n", ret);
		goto fail_after_gsensor;
	}

	ret = ft_power_info_power_status_update();
	if (ret) {
		pr_err("phytium ec: failed to read power status: %d\n", ret);
		goto fail_after_gsensor;
	}

	if (power_info->bat_in) {
		/* Get battery static information. */
		ret = ft_power_info_battery_static_update();
		if (ret) {
			pr_err("phytium ec: failed to read static information: %d\n",
			       ret);
			goto fail_after_gsensor;
		}
	} else {
		pr_info("phytium Platform Driver: The battery does not exist!!\n");
	}

	/*
	 * Register both supplies whenever the chassis has them, whether or
	 * not a battery is fitted right now: PRESENT reports the absence and
	 * the handlers must never be left with a NULL supply.
	 */
	if (chassis_types_is_laptop()) {
		ec_bat = devm_power_supply_register(&dev->dev, &ft_bat, NULL);
		if (IS_ERR(ec_bat)) {
			ret = PTR_ERR(ec_bat);
			ec_bat = NULL;
			goto fail_after_gsensor;
		}

		ec_ac = devm_power_supply_register(&dev->dev, &ft_ac, NULL);
		if (IS_ERR(ec_ac)) {
			ret = PTR_ERR(ec_ac);
			ec_ac = NULL;
			goto fail_after_gsensor;
		}
	}
	/* Register power supply END */

	/* Register Temp and Fan hwmon */
	ec_sensors_register(dev);

	gpio_irq = acpi_dev_gpio_irq_get(ACPI_COMPANION(&(dev->dev)), 0);
	if (gpio_irq == -EPROBE_DEFER) {
		/* The GPIO controller is not ready; do not fall back to LPC. */
		ret = -EPROBE_DEFER;
		goto fail_after_gsensor;
	}

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
	ft_ec_irq_exit();
	ec_gsensor_exit();
	ft_hotkey_exit();
	return ret;
}

static int ft_lpc_event_remove(struct platform_device *dev)
{
	ft_ec_irq_exit();
	ec_gsensor_exit();
	ft_hotkey_exit();

	/*
	 * power_info, the power supplies and the backlight stay valid until
	 * devres releases them, because their callbacks may still run while
	 * the device is being torn down.
	 */
	return 0;
}

module_platform_driver(ft_ec_pdriver);

MODULE_DESCRIPTION("phytium Laptop Driver");
MODULE_LICENSE("GPL");
