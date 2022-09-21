#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include <linux/device.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>

MODULE_AUTHOR("Billy");
MODULE_DESCRIPTION("phytium clock driver");
MODULE_LICENSE("GPL");

static int ft_clock_probe(struct platform_device *dev)
{
    struct clk *clk;
    int rate;

    device_property_read_u32(&dev->dev,"clock-frequency", &rate);
    clk = clk_register_fixed_rate(&dev->dev, dev_name(&dev->dev), NULL, 0, rate);
    clk_register_clkdev(clk, NULL, dev_name(&dev->dev));

    return 0;
}

static const struct acpi_device_id ft_clock_acpi_match[] = {
    { "FTCK0002", 0 },
    { "FTCK0003", 0 },
    { "PHYT8002", 0 },
    { "KYLN0008", 0 },
    {}
};
MODULE_DEVICE_TABLE(acpi, lpc_acpi_match);

static struct platform_driver ft_clock_driver = {
    .driver		= {
        .name	= "phytium clock",
        .acpi_match_table = ACPI_PTR(ft_clock_acpi_match),
    },
    .probe		= ft_clock_probe,
};

static int __init ft_clock_init(void)
{
    int ret;
    ret = platform_driver_register(&ft_clock_driver);
    if (ret){
        pr_info("platform_driver_register fail\n");
        return 0;
    }

    return 0;

}
subsys_initcall(ft_clock_init);

static void __exit ft_clock_exit(void)
{
    platform_driver_unregister(&ft_clock_driver);
}
module_exit(ft_clock_exit);
