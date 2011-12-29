/*
 * ewdt.c
 *
 *  Created on: 29.12.2011
 *      Author: Alex AVAlon
 */

#define DEBUG

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
#include <linux/reboot.h>
#include <asm/uaccess.h>
#include <mach/at91sam9_smc.h>


#define TICKSMAX		2

static struct platform_device *ewdt_device;
volatile static char fexit = 0;
int nmajor;

static int ewdt_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
	printk(KERN_INFO "file_open (0x%X)\n",file);
#endif
    return 0;
}

static ssize_t ewdt_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
	unsigned int  i;

	for (i=0; i < 5; i++) put_user((char*)"ewdt\0", (char __user *) (buffer + i));

#ifdef DEBUG
	printk(KERN_INFO "file_read (0x%X)\n",file);
#endif

	return 5;
}

static ssize_t ewdt_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
unsigned char num;

	copy_from_user(&num, (const char __user *) buffer, 1);

#ifdef DEBUG
	printk(KERN_INFO "file_write (0x%X)\n",file);
#endif
	return length;
}

static int ewdt_flush (struct file *file, fl_owner_t id){
#ifdef DEBUG
	printk(KERN_INFO "file_flush (0x%X)\n",file);
#endif

	return 0;
}

static int ewdt_release(struct inode *inode, struct file *file){
#ifdef DEBUG
	printk(KERN_INFO "file_release (0x%X)\n",file);
#endif

	return 0;
}

static struct file_operations ewdt_fops = {
	.owner	= THIS_MODULE,
	.open	= ewdt_open,
	.read	= ewdt_read,
	.write	= ewdt_write,
	.release  = ewdt_release,
	.flush = ewdt_flush,
};

struct timer_list ewdt_timer;
static unsigned char counter = TICKSMAX;
void ewdt_timer_func(unsigned long data){

	if (!counter) counter = TICKSMAX;
	counter--;

	if (!fexit)	mod_timer(&ewdt_timer, jiffies + HZ/TICKSMAX);
}

static int ewdt_probe (struct platform_device *pdev)	// -- for platform devs
{
	int i;
	int ret;

	nmajor = 131;
	ret = register_chrdev(nmajor, "ewdt", &ewdt_fops);
	if (ret < 0){
		return ret;
	}

    ewdt_device = pdev;

	init_timer(&ewdt_timer);
	ewdt_timer.expires = jiffies + HZ/TICKSMAX;
	ewdt_timer.data = 0;
	ewdt_timer.function = ewdt_timer_func;
	add_timer(&ewdt_timer);

	return 0;
}

static int __exit ewdt_remove(struct platform_device *pdev)
{

	if (platform_get_drvdata(pdev)) {
		printk(KERN_INFO "ewdt removed. OK.\n");
	}else printk(KERN_INFO "ewdt don't removed. False.\n");

	return 0;
}


static struct platform_driver ewdt_driver = {
	.remove = __exit_p(ewdt_remove),

	.driver = {
		.name = "ewdt",
		.owner = THIS_MODULE,
	},
};

static int __init ewdt_init(void)
{
	int ret;

	ret = platform_driver_probe(&ewdt_driver, ewdt_probe);

	if (ret) {
		// В случае когда девайс еще не добавлен
		platform_driver_unregister(&ewdt_driver);
		return -ENODEV;
	}

	return 0;
}

static void __exit ewdt_exit(void)
{
	fexit = 1;
	del_timer_sync(&ewdt_timer);

	unregister_chrdev(nmajor, "ewdt");
	platform_driver_unregister(&ewdt_driver);
	printk(KERN_INFO "device_closed\n");
}

module_init(ewdt_init);
module_exit(ewdt_exit);

MODULE_AUTHOR("Alex AVAlon");
MODULE_SUPPORTED_DEVICE("ledsrelays");
MODULE_LICENSE("GPL");

