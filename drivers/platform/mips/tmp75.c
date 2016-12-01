#include <linux/module.h>

static int __init tmp75_init(void)
{
	printk("====Please use device-tree for TMP75 Driver====\n");
	printk("You can put the tmp75 node under the i2c node:\n"
	       "tmp75@4e {\n"
	       "	compatible = \"ti,tmp75\"\n"
	       "	reg = <0x4e>\n"
	       "}\n");
	printk("==================Thank you!===================\n");
	return 0;
}

static void __exit tmp75_exit(void)
{
}

late_initcall(tmp75_init);
module_exit(tmp75_exit);

MODULE_AUTHOR("Hongbing Hu <huhb@lemote.com>");
MODULE_DESCRIPTION("TMP75 driver, based on the EMC1412 driver");
MODULE_LICENSE("GPL");
