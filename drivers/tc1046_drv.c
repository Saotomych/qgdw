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
#include <linux/interrupt.h>

#include <asm/uaccess.h>
#include <mach/at91sam9_smc.h>

#include "at91_adc.h"

static struct resource *adcmem_rc;
static struct resource *adcirq_rc;
static unsigned char __iomem *adcio;

static struct platform_device *adc_device;
int nmajor;

static irqreturn_t get_temper_tc1046(int irq, void *dev_id)
{
	printk(KERN_INFO "adc interrupt %d cause\n", irq);

	return IRQ_HANDLED;
}

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

	nmajor = 130;
	ret = register_chrdev(nmajor, "temper", &adc_fops);
	if (ret < 0){
		return ret;
	}

    adc_device = pdev;

    // Getting resources
	adcmem_rc = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	adcirq_rc = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	adcio = ioremap(adcmem_rc->start, 0);

	// Register IRQ
	request_irq(adcirq_rc->start, get_temper_tc1046, 0, "temper", pdev);

	return 0;
}

static int __exit adc_remove(struct platform_device *pdev)
{

	if (platform_get_drvdata(pdev)) {
		printk(KERN_INFO "driver removed. OK.\n");
	}else printk(KERN_INFO "driver don't removed. False.\n");

	return 0;
}

static struct platform_driver adc_driver = {
	.remove = __exit_p(adc_remove),

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
