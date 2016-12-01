#include <linux/module.h>

static int __init at24c04_init(void)
{
	printk("====Please use device-tree for AT24 Driver====\n");
	printk("You can put the at24 node under the i2c node:\n"
	       "eeprom@51 {\n"
	       "	compatible = \"atmel,24c04\"\n"
	       "	reg = <0x51>\n"
	       "	size = <512>\n"
	       "	pagesize = <16>\n"
	       "}\n");
	printk("==================Thank you!==================\n");
	return 0;
}

static void __exit at24c04_exit(void)
{
}

module_init(at24c04_init);
module_exit(at24c04_exit);

MODULE_AUTHOR("Binbin Zhou <zhoubb@lemote.com>");
MODULE_DESCRIPTION("AT24c04 driver, based on the at24 driver");
MODULE_LICENSE("GPL");
