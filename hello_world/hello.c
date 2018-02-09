#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
static int __init myinit(void)
{
	printk(KERN_ALERT "Hello, world\n");
	return 0;
}
static void __exit myexit(void)
{
	printk(KERN_ALERT "Goodbye, World\n");
}
module_init(myinit);
module_exit(myexit);
MODULE_LICENSE("GPL");
