/*
 * ledsrelays.c
 *
 *  Created on: 10.05.2011
 *      Author: alex
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <mach/at91sam9_smc.h>

enum{led1, led2, rel1, rel2};
static struct resource *lr_resources[4];
static unsigned char *io_dat[4];
static struct platform_device *lr_device;

int nmajor;

static int lr_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "file_open (%s)\n",file);

    return 0;
}

static ssize_t lr_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
	unsigned int len=0, i;
	unsigned char *prddata = 0;

	return len;
}

static ssize_t lr_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
	return 0;
}

static int lr_flush (struct file *file, fl_owner_t id){

	return 0;
}

static int lr_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "file_release (%s)\n",file);

	return 0;
}

static struct file_operations lr_fops = {
	.owner	= THIS_MODULE,
	.open	= lr_open,
	.read	= lr_read,
	.write	= lr_write,
	.release  = lr_release,
};

static int lr_probe (struct platform_device *pdev)	// -- for platform devs
{
	int ret;
	ret = register_chrdev(0, "ledsrelays", &lr_fops);
	if (ret < 0){
		return ret;
	}
	nmajor = ret;

    lr_device = pdev;

    // Getting resources
	lr_resources[led1] = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	lr_resources[led2] = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	lr_resources[rel1] = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	lr_resources[rel2] = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	// Registration I/O mem for registers
	io_dat[led1] = ioremap(lr_resources[led1]->start, 1);
	io_dat[led2] = ioremap(lr_resources[led2]->start, 1);
	io_dat[rel1] = ioremap(lr_resources[rel1]->start, 1);
	io_dat[rel2] = ioremap(lr_resources[rel2]->start, 1);

	return 0;
}

static int __exit lr_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);

	if (info) {
		printk(KERN_INFO "fb removed. OK.\n");
	}else printk(KERN_INFO "fb don't removed. False.\n");

	return 0;
}

static struct platform_driver lr_driver = {
	.remove = __exit_p(lr_remove),

	.driver = {
		.name = "ledsrelays",
		.owner = THIS_MODULE,
	},
};

static int __init lr_init(void)
{
	int ret;
	ret = platform_driver_probe(&lr_driver, lr_probe);

	if (ret) {
		// В случае когда девайс еще не добавлен
		platform_driver_unregister(&lr_driver);
		return -ENODEV;
	}
	return 0;
}

static void __exit lr_exit(void)
{
	unregister_chrdev(nmajor, "ledsrelays");
	platform_driver_unregister(&lr_driver);
	printk(KERN_INFO "device_closed\n");
}

module_init(lr_init);
module_exit(lr_exit);

MODULE_AUTHOR("alex AAV");
MODULE_SUPPORTED_DEVICE("ledsrelays");
MODULE_LICENSE("GPL");
