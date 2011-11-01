/*
 * tc1046_drv.c
 *
 *  Created on: 01.11.2011
 *      Author: Alex AVAlon
 *
 *      Driver for High Precision Temperature-to-Voltage Converter TC1046
 *
 *      Default ADC - AD2 connected to pin PC2 for M700 board
 *
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
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <asm/uaccess.h>
#include <mach/at91sam9_smc.h>

#include "at91_adc.h"

static struct platform_device *adc_device;
int nmajor;

static int adc_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "file_open (0x%X)\n",file);

    return 0;
}

static ssize_t adc_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
int ret = 0;

	return ret;
}

static struct file_operations adc_fops = {
	.owner	= THIS_MODULE,
	.open	= adc_open,
	.read	= adc_read,
};

static int adc_probe (struct platform_device *pdev)	// -- for platform devs
{
	int i;
	int ret;
	ret = register_chrdev(127, "adc", &adc_fops);
	if (ret < 0){
		return ret;
	}
	nmajor = 138;

    adc_device = pdev;

    // Getting resources

	return 0;
}

static struct platform_driver adc_driver = {
//	.remove = __exit_p(tc1046_remove),

	.driver = {
		.name = "tc1046",
		.owner = THIS_MODULE,
	},
};


static int __init tc1046_init(void)
{
	int ret;

	ret = platform_driver_probe(&adc_driver, adc_probe);
//
//	if (ret) {
//		// В случае когда девайс еще не добавлен
//		platform_driver_unregister(&lr_driver);
//		return -ENODEV;
//	}

	return 0;
}

static void __exit tc1046_exit(void)
{
	unregister_chrdev(nmajor, "tc1046");
	platform_driver_unregister(&adc_driver);
	printk(KERN_INFO "device_closed\n");
}

module_init(tc1046_init);
module_exit(tc1046_exit);

MODULE_AUTHOR("Alex AVAlon");
MODULE_SUPPORTED_DEVICE("ledsrelays");
MODULE_LICENSE("GPL");
