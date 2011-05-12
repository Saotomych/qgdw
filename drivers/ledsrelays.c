/*
 *  ledsrelays.c
 *  Created on: 10.05.2011
 *      Author: alex AAV
 *
 * Driver for platform_device ledsrels realized onboard M700
 * ledsrels have 4 8-bits output registers
 * 1 register controls 4 2-color leds
 * 2 register controls 4 1-color leds
 * 3 register controls 3 relays
 * 4 register controls 2 relays & GPRS controls signals
 *
 * Incoming data bit format (one register)
 * 0 byte != 0xFF
 * 0 byte - register's number (0-1 bit, 2-7 - ignored)
 * 1-8 byte - value of bit (0 bit, 1-7 - ignored)
 *
 * Incoming data byte format (all registers)
 * 0 byte == 0xFF
 * 1 byte - 0-7 bits - state register 0
 * 2 byte - 0-7 bits - state register 1
 * 3 byte - 0-7 bits - state register 2
 * 4 byte - 0-7 bits - state register 3
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
static unsigned char __iomem *io_dat[4];
static unsigned char realstate[4];
static struct platform_device *lr_device;

int nmajor;

static int lr_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "file_open (0x%X)\n",file);

    return 0;
}

static ssize_t lr_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
	unsigned int  i;
	unsigned char *prddata = 0;

	for (i=0; i < 4; i++) put_user(realstate[i], (char __user *) (buffer + i));

	printk(KERN_INFO "file_read (0x%X)\n",file);

	return 4;
}

static ssize_t lr_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
unsigned char data = 0, mask = 1, tmpd;
unsigned char num, i, rlen = 9;

	copy_from_user(&num, (const char __user *) buffer, 1);

	if (num == 0xFF){
		// Byte format
		for (i = 1; i < 5; i++){
			copy_from_user(&tmpd, (const char __user *) (buffer + i), 1);
			*io_dat[i-1] = tmpd;
			realstate[i-1] = tmpd;
		}
		printk(KERN_INFO "file_write (0x%X); data 0x%X; 0x%X; 0x%X; 0x%X.\n",file, realstate[0], realstate[1], realstate[2], realstate[3]);
	}else{
		// Bit format
		num &= 0x3;
		if (length < 9) rlen = length;
		for (i = 1; i < rlen; i++){
			copy_from_user(&tmpd, (const char __user *) (buffer + i), 1);
			tmpd &= 1;
			data |= (tmpd ? mask : 0);
			mask <<=1;
		}
		printk(KERN_INFO "file_write (0x%X); num 0x%X; data 0x%X; in_len %d; len %d\n",file, num, data, length, rlen);
		*io_dat[num] = data;
		realstate[num] = data;
	}

	return length;
}

static int lr_flush (struct file *file, fl_owner_t id){
	printk(KERN_INFO "file_flush (0x%X)\n",file);

	return 0;
}

static int lr_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "file_release (0x%X)\n",file);

	return 0;
}

static struct file_operations lr_fops = {
	.owner	= THIS_MODULE,
	.open	= lr_open,
	.read	= lr_read,
	.write	= lr_write,
	.release  = lr_release,
	.flush = lr_flush,
};

static int lr_probe (struct platform_device *pdev)	// -- for platform devs
{
	int ret;
	ret = register_chrdev(127, "ledsrelays", &lr_fops);
	if (ret < 0){
		return ret;
	}
	nmajor = 127;

    lr_device = pdev;

    // Getting resources
	lr_resources[led1] = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	lr_resources[led2] = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	lr_resources[rel1] = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	lr_resources[rel2] = platform_get_resource(pdev, IORESOURCE_MEM, 3);

    printk(KERN_INFO "set i/o NCS5: 0x%lX; 0x%lX; 0x%lX; 0x%lX\n", lr_resources[led1], lr_resources[led2], lr_resources[rel1], lr_resources[rel2]);

	// Registration I/O mem for registers
	io_dat[led1] = ioremap(lr_resources[led1]->start, 1);
	io_dat[led2] = ioremap(lr_resources[led2]->start, 1);
	io_dat[rel1] = ioremap(lr_resources[rel1]->start, 1);
	io_dat[rel2] = ioremap(lr_resources[rel2]->start, 1);

	*io_dat[0] = 0; realstate[0] = 0;
	*io_dat[1] = 0; realstate[1] = 0;
	*io_dat[2] = 0; realstate[2] = 0;
	*io_dat[3] = 0; realstate[3] = 0;

	printk(KERN_INFO "set virt i/o: 0x%lX; 0x%lX; 0x%lX; 0x%lX\n", io_dat[led1], io_dat[led2], io_dat[rel1], io_dat[rel2]);

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
		.name = "ledsrels",
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
