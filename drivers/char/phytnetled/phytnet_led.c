// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022-2023 Phytium Technology Co.,Ltd.
 *
 */
#include <linux/acpi.h>
#include <linux/netdevice.h>
#include <linux/of_platform.h>
#include <linux/of_net.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/slab.h>
#include "phytnet_led.h"

#define DRIVER_NAME "phytnet_led"
#define DRIVER_VERSION "1.0"
#define DRIVER_AUTHOR "LongShixiang <longshixiang1718@phytium.com.cn>"
#define DRIVER_DESC "net device led control module"
#define NET_DEV_PROPNAME "net_dev"
#define LED_OF_NAME "led"
#define CHECK_INTERVAL 125 /* Unit: ms */
#define NDEV_CHECK_DELAY	30000  /* Unit: 30s */
#define LED_ON	1
#define LED_OFF	0
#define LINK_OFFSET 0
#define ACT_OFFSET 1

#if defined(CONFIG_OF)
static const struct of_device_id phytnet_led_of_ids[] = {
	{ .compatible = "phytium,net_led"},
	{}
};

MODULE_DEVICE_TABLE(of, phytnet_led_of_ids);
#endif /* CONFIG_OF */

#ifdef CONFIG_ACPI
static const struct acpi_device_id phytnet_acpi_ids[] = {
	{ .id = "PHYT800C"},
	{}
};
MODULE_DEVICE_TABLE(acpi, phytnet_acpi_ids);
#else
#define phytnet_acpi_ids NULL
#endif /* CONFIG_ACPI */

static void
led_on(struct gpio_desc *gd)
{
	gpiod_set_value(gd, LED_ON);
}

static void
led_off(struct gpio_desc *gd)
{
	gpiod_set_value(gd, LED_OFF);
}

static void
led_blink(struct led_data *phytnet_led)
{
	phytnet_led->act_val = !phytnet_led->act_val;
	gpiod_set_value(phytnet_led->act, phytnet_led->act_val);
}

static int
port_is_linkup(struct led_data *phytnet_led)
{
	if (netif_carrier_ok(phytnet_led->ndev))
		return true;
	else
		return false;
}

static bool
port_is_act(struct led_data *phytnet_led)
{
	bool ret = false;

	if (phytnet_led->ndev_rx != phytnet_led->ndev->stats.rx_packets) {
		phytnet_led->ndev_rx = phytnet_led->ndev->stats.rx_packets;
		ret = true;
	}

	if (phytnet_led->ndev_tx != phytnet_led->ndev->stats.tx_packets) {
		phytnet_led->ndev_tx = phytnet_led->ndev->stats.tx_packets;
		ret = true;
	}

	return ret;
}

static void
led_control(struct led_data *phytnet_led)
{
	while (!phytnet_led->led_stop) {
		msleep(CHECK_INTERVAL);

		if (!netif_running(phytnet_led->ndev)) {
			led_off(phytnet_led->link);
			led_off(phytnet_led->act);
			continue;
		}

		if (port_is_linkup(phytnet_led))
			led_on(phytnet_led->link);
		else
			led_off(phytnet_led->link);

		if (port_is_act(phytnet_led))
			led_blink(phytnet_led);
		else
			led_off(phytnet_led->act);
	}
}

static int
of_ndev_init(struct led_data *phytnet_led)
{
	struct device_node *net_node;

	net_node = of_parse_phandle(phytnet_led->pdev->dev.of_node, NET_DEV_PROPNAME, 0);
	if (!net_node) {
		dev_err(&phytnet_led->pdev->dev, "Failed to get netdev ofnode from device tree\n");
		return -ENODEV;
	}

	phytnet_led->ndev = of_find_net_device_by_node(net_node);

	if (!phytnet_led->ndev) {
		dev_err(&phytnet_led->pdev->dev, "Failed to get acpi ndev\n");
		return -ENODEV;
	}

	dev_info(&phytnet_led->pdev->dev, "Successfully get ndev...\n");
	dev_hold(phytnet_led->ndev);

	return 0;
}


static int
acpi_ndev_init(struct led_data *phytnet_led)
{
	int err;
	struct net_device *find_ndev;
	const char *ndev_acpi_path;
	acpi_handle net_handler;
	struct acpi_device *adev;
	acpi_status status;
	struct device *find_dev;

	err = device_property_read_string(&phytnet_led->pdev->dev,
					NET_DEV_PROPNAME,
					&ndev_acpi_path);
	if (err) {
		dev_err(&phytnet_led->pdev->dev, "Failed to read net_dev property!\n");
		return -ENODEV;
	}

	status = acpi_get_handle(NULL, (acpi_string)ndev_acpi_path, &net_handler);
	if (ACPI_FAILURE(status)) {
		dev_err(&phytnet_led->pdev->dev, "Failed to get acpi handler  path: %s\n",
			ndev_acpi_path);
		return -ENODEV;
	}

	adev = acpi_get_acpi_dev(net_handler);
	if (!err) {
		dev_err(&phytnet_led->pdev->dev, "Failed to get adev dev\n");
		return -ENODEV;
	}

	for_each_netdev(&init_net, find_ndev) {
		if (find_ndev->dev.parent != NULL) {
			find_dev = find_ndev->dev.parent;
			if (&adev->fwnode == find_dev->fwnode)
				phytnet_led->ndev = find_ndev;
		}
	}

	if (!phytnet_led->ndev) {
		dev_err(&phytnet_led->pdev->dev, "Failed to get acpi ndev\n");
		return -ENODEV;
	}

	dev_info(&phytnet_led->pdev->dev, "Successfully get ndev...\n");
	dev_hold(phytnet_led->ndev);

	return 0;
}

static int
gpio_init(struct led_data *phytnet_led)
{
	int err;

	phytnet_led->link = gpiod_get_index(&phytnet_led->pdev->dev,
					LED_OF_NAME,
					LINK_OFFSET,
					GPIOD_OUT_HIGH);
	if (IS_ERR(phytnet_led->link)) {
		dev_err(&phytnet_led->pdev->dev, "Failed to get link gpio, err: %ld\n",
			PTR_ERR(phytnet_led->link));
		return PTR_ERR(phytnet_led->link);
	}

	err = gpiod_direction_output(phytnet_led->link, LED_OFF);
	if (err) {
		dev_err(&phytnet_led->pdev->dev, "Failed to set link dir, err: %ld\n",
			PTR_ERR(phytnet_led->link));
		return err;
	}

	phytnet_led->act = gpiod_get_index(&phytnet_led->pdev->dev,
					LED_OF_NAME,
					ACT_OFFSET,
					GPIOD_OUT_HIGH);
	if (IS_ERR(phytnet_led->act)) {
		dev_err(&phytnet_led->pdev->dev, "Failed to get act gpio, err:%d\n", err);
		return PTR_ERR(phytnet_led->act);
	}

	err = gpiod_direction_output(phytnet_led->act, LED_OFF);
	if (err) {
		dev_err(&phytnet_led->pdev->dev, "Failed to set act dir, err: %d\n", err);
		return err;
	}

	return 0;
}

static void
led_init_and_control(struct work_struct *work)
{
	int err = -1;
	struct led_data *phytnet_led = container_of(work, struct led_data, led_control_work.work);

	if (phytnet_led->pdev->dev.of_node)
		err = of_ndev_init(phytnet_led);
	else if (has_acpi_companion(&phytnet_led->pdev->dev))
		err = acpi_ndev_init(phytnet_led);

	if (err) {
		dev_err(&phytnet_led->pdev->dev, "ndev init wrong\n");
		return;
	}

	err = gpio_init(phytnet_led);
	if (err) {
		dev_err(&phytnet_led->pdev->dev, "gpio init wrong\n");
		return;
	}

	led_control(phytnet_led);
}

static int
net_led_probe(struct platform_device *pdev)
{
	struct led_data *phytnet_led = devm_kzalloc(&pdev->dev,
						sizeof(struct led_data),
						GFP_KERNEL);

	if (!phytnet_led)
		return -ENOMEM;

	platform_set_drvdata(pdev, phytnet_led);

	phytnet_led->act = LED_OFF;
	phytnet_led->pdev = pdev;
	phytnet_led->led_stop = 0;

	INIT_DELAYED_WORK(&phytnet_led->led_control_work, led_init_and_control);
	schedule_delayed_work(&phytnet_led->led_control_work, msecs_to_jiffies(NDEV_CHECK_DELAY));

	return 0;
}

static int
net_led_remove(struct platform_device *pdev)
{
	struct led_data *phytnet_led = platform_get_drvdata(pdev);

	phytnet_led->led_stop = 1;
	cancel_delayed_work_sync(&phytnet_led->led_control_work);

	if (phytnet_led->ndev)
		dev_put(phytnet_led->ndev);

	if (phytnet_led->link) {
		led_off(phytnet_led->link);
		gpiod_put(phytnet_led->link);
	}

	if (phytnet_led->act) {
		led_off(phytnet_led->act);
		gpiod_put(phytnet_led->act);
	}

	devm_kfree(&pdev->dev, phytnet_led);

	return 0;
}

static struct platform_driver net_led_driver = {
	.driver = {
			.owner = THIS_MODULE,
			.name = DRIVER_NAME,
			.of_match_table = of_match_ptr(phytnet_led_of_ids),
			.acpi_match_table = ACPI_PTR(phytnet_acpi_ids),
		},
	.probe = net_led_probe,
	.remove = net_led_remove,
};

module_platform_driver(net_led_driver);

MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_ALIAS("platform:" DRIVER_NAME);
