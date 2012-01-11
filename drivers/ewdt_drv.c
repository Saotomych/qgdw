/*
 * ewdt.c
 *
 *  Created on: 29.12.2011
 *      Author: Alex AVAlon
 */

//#define DEBUG

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
#define REBOOT_TIME		5

static struct platform_device *ewdt_device;
volatile static char ewdt_fexit = 0;
static int ewdt_nmajor;

// Application define dimension
struct{
	unsigned int apppid;
	unsigned int time;
} apptime[30];

int appmax = 0;

static int ewdt_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
	printk(KERN_INFO "ewdt: file_open (0x%X)\n",file);
#endif
    return 0;
}

static ssize_t ewdt_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
	unsigned int  i;

	for (i=0; i < 5; i++) put_user((char*)"ewdt\0", (char __user *) (buffer + i));

#ifdef DEBUG
	printk(KERN_INFO "ewdt: file_read (0x%X)\n",file);
#endif

	return 5;
}

static ssize_t ewdt_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
unsigned char num, i, all, idx;
unsigned int pid=0, cmd=0;

	copy_from_user(&num, (const char __user *) buffer, 1);

#ifdef DEBUG
	printk(KERN_INFO "ewdt: file_write (0x%X), num=%d\n", file, num);
#endif

//	if (!(num & 7)){

		// For some commands in one copy_from_user
		all = num >> 2;
		while(all){

			all--;
			idx = all << 2;
			pid = ((unsigned int)buffer[idx+1]) | ((unsigned int) (buffer[idx+2] << 8));
			cmd = buffer[idx+3];

#ifdef DEBUG
			printk(KERN_INFO "ewdt: data for wdt control len=%d; pid=0x%04X; cmd=%d\n", num, pid, cmd);
#endif

			for(i=0; i < appmax; i++){
				if (apptime[i].apppid == pid) break;
			}

			if (i == appmax){
				// new application registered
				if (appmax < (30-1)){

#ifdef DEBUG
					printk(KERN_INFO "ewdt: new application registered, pid = 0x%04X\n", pid);
#endif

					apptime[appmax].apppid = pid;
					apptime[appmax].time = 0;
					appmax++;
				}
			}

			if (cmd == 1){
				// reset application time

#ifdef DEBUG
				printk(KERN_INFO "ewdt: reset application time for pid 0x%04X == 0x%04X\n", pid, apptime[i].apppid);
#endif

				apptime[i].time = 0;
			}

			if ((cmd == 2) && (i < appmax)){
				// application unregistered
#ifdef DEBUG
				printk(KERN_INFO "ewdt: application unregistered 0x%04X == 0x%04X\n", pid, apptime[i].apppid);
#endif
				if (appmax > 1){
					apptime[i].apppid = apptime[appmax-1].apppid;
					apptime[i].time = apptime[appmax-1].time;
				}
				if (appmax)	appmax--;
#ifdef DEBUG
				printk(KERN_INFO "ewdt: application changed to 0x%04X\n", apptime[i].apppid);
#endif
			}

		}
//	}

	return length;
}

static int ewdt_flush (struct file *file, fl_owner_t id){
#ifdef DEBUG
	printk(KERN_INFO "ewdt: file_flush (0x%X)\n",file);
#endif

	return 0;
}

static int ewdt_release(struct inode *inode, struct file *file){
#ifdef DEBUG
	printk(KERN_INFO "ewdt: file_release (0x%X)\n",file);
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
int i;

	// set reset_pin of wdt
	at91_set_gpio_value(AT91_PIN_PC10, 1);

	// add times and time test and delay for reset signal
	if (!counter){
		counter = TICKSMAX;

		for(i=0; i < appmax; i++){
#ifdef DEBUG
			printk(KERN_INFO "ewdt: apptime[%d].time = %d\n", i, apptime[i].time);
#endif
			if (apptime[i].time > REBOOT_TIME){
#ifdef DEBUG
				printk(KERN_INFO "ewdt: Restart must have here\n");
				apptime[i].time=0;
#endif
				printk(KERN_CRIT "ewdt: Initiating system reboot.\n");
				emergency_restart();
				printk(KERN_CRIT "ewdt: Reboot didn't ?????\n");
			}
			apptime[i].time++;
		}
	}

	// reset reset_pin of wdt
	at91_set_gpio_value(AT91_PIN_PC10, 0);

	counter--;
	if (!ewdt_fexit) mod_timer(&ewdt_timer, jiffies + HZ);
}

static int __init ewdt_init(void)
{
	int ret = 0;

	ewdt_nmajor = 131;
	ret = register_chrdev(ewdt_nmajor, "ewdt", &ewdt_fops);
	if (ret < 0){
		return ret;
	}

    init_timer(&ewdt_timer);
	ewdt_timer.expires = jiffies + HZ/TICKSMAX;
	ewdt_timer.data = 0;
	ewdt_timer.function = ewdt_timer_func;
	add_timer(&ewdt_timer);

	printk(KERN_INFO "external wdt: Init end\n");

	return 0;
}

static void __exit ewdt_exit(void)
{
	ewdt_fexit = 1;
	del_timer_sync(&ewdt_timer);

	unregister_chrdev(ewdt_nmajor, "ewdt");
	printk(KERN_INFO "external wdt: device_closed\n");
}

module_init(ewdt_init);
module_exit(ewdt_exit);

MODULE_AUTHOR("Alex AVAlon");
MODULE_SUPPORTED_DEVICE("ewdt");
MODULE_LICENSE("GPL");

