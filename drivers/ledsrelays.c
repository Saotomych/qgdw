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
 * 1-8 byte - for leds - value of bit with pulse data values: 0 - off, 1-6 - pulse, 7 - on
 * 1-8 byte - for relays - values: 0 - off, 1 - on
 *
 * 12.05.2011 - release-1.0
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

#define TICKSMAX		7

enum{led1, led2, rel1, rel2};
static struct resource *lr_resources[4];
static unsigned char __iomem *io_dat[4];
static unsigned char realstate[32];		// реальное состояние и тики мигания светодиода
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

	for (i=0; i < 32; i++) put_user(realstate[i], (char __user *) (buffer + i));

	printk(KERN_INFO "file_read (0x%X)\n",file);

	return 32;
}

static ssize_t lr_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
unsigned char data = 0, mask = 1, tmpd;
unsigned char num, pos, i, rlen = 9;

	copy_from_user(&num, (const char __user *) buffer, 1);

		num &= 0x3; pos = num<<3;
		if (length < 9) rlen = length;
		for (i = 1; i < rlen; i++){
			copy_from_user(&tmpd, (const char __user *) (buffer + i), 1);
			realstate[pos+i-1] = tmpd & 7;
			tmpd &= 1;
			data |= (tmpd ? mask : 0);
			mask <<=1;
		}
		printk(KERN_INFO "file_write (0x%X); num 0x%X; data 0x%X; in_len %d; len %d\n",file, num, data, length, rlen);
		*io_dat[num] = data;

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

struct timer_list led_timer;
static unsigned char counter = TICKSMAX;
void led_timer_func(unsigned long data){
unsigned int i;
unsigned char leds, mask;

	if (!counter) counter = TICKSMAX;

	mask = 1; leds = 0;
	for (i=0; i<8; i++){
		if (realstate[i] >= counter) leds |= mask;
		mask <<= 1;
	}
	*io_dat[led1] = leds;

	mask = 1; leds = 0;
	for (i=8; i<16; i++){
		if (realstate[i] >= counter) leds |= mask;
		mask <<= 1;
	}
	*io_dat[led2] = leds;

	counter--;
	mod_timer(&led_timer, jiffies + HZ/TICKSMAX);
}

static int lr_probe (struct platform_device *pdev)	// -- for platform devs
{
	int i;
	int ret;

	nmajor = 127;
	ret = register_chrdev(nmajor, "ledsrelays", &lr_fops);
	if (ret < 0){
		return ret;
	}

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

	init_timer(&led_timer);
	led_timer.expires = jiffies + HZ/TICKSMAX;
	led_timer.data = 0;
	led_timer.function = led_timer_func;
	add_timer(&led_timer);

	printk(KERN_INFO "set virt i/o: 0x%lX; 0x%lX; 0x%lX; 0x%lX\n", io_dat[led1], io_dat[led2], io_dat[rel1], io_dat[rel2]);

	return 0;
}

static int __exit lr_remove(struct platform_device *pdev)
{

	if (platform_get_drvdata(pdev)) {
		printk(KERN_INFO "lr removed. OK.\n");
	}else printk(KERN_INFO "lr don't removed. False.\n");

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

MODULE_AUTHOR("Alex AVAlon");
MODULE_SUPPORTED_DEVICE("ledsrelays");
MODULE_LICENSE("GPL");
