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
#include <mach/at91_adc.h>
#include <mach/at91_pmc.h>
#include <mach/io.h>

#include "AT91SAM9260_inc.h"

#define DEBUG

static struct resource *adcmem_rc;
static struct resource *adcirq_rc;
static unsigned char __iomem *adcio;

static unsigned int lastdata;

#define u32_io unsigned int __iomem

//static unsigned long ADC_OFFSET;

static struct platform_device *adc_device;
int nmajor;

static irqreturn_t get_temper_tc1046(int irq, void *dev_id)
{
unsigned int imask, stat, dat;
	stat = readl(adcio + ADC_SR);
	dat = readl(adcio + ADC_CDR2);
	lastdata = dat;

#ifdef DEBUG
	imask = readl(adcio + ADC_IMR);
	printk(KERN_INFO "dc1046: adc mask int = 0x%04X\n", imask);
	printk(KERN_INFO "dc1046: adc status in int = 0x%04X\n", stat);
	printk(KERN_INFO "dc1046: adc data = 0x%03X\n", dat);
#endif

	return IRQ_HANDLED;
}

static int adc_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
	printk(KERN_INFO "dc1046: file_open (0x%X)\n",file);
#endif
    return 0;
}

static ssize_t adc_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
char s[32];
int l, i;

	sprintf(s, "dc1046: temperature %d\n", lastdata);
	l = strlen(s);
	for (i=0; i < l; i++) put_user(s[i], (char __user *) (buffer + i));

	// Start AD conversion for get actual temperature
	writel(AT91C_ADC_START, adcio + ADC_CR);

	return l;
}

static ssize_t adc_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
#ifdef DEBUG
	printk(KERN_INFO "dc1046: file_write (0x%X)\n",file);
#endif
	return length;
}

static int adc_release(struct inode *inode, struct file *file)
{
#ifdef DEBUG
	printk(KERN_INFO "dc1046: file_release (0x%X)\n",file);
#endif
	return 0;
}


static struct file_operations adc_fops = {
	.owner	= THIS_MODULE,
	.open	= adc_open,
	.read	= adc_read,
	.write	= adc_write,
	.release  = adc_release,
};

static int adc_probe (struct platform_device *pdev)	// -- for platform devs
{
int ret;

	nmajor = 130;
	ret = register_chrdev(nmajor, "temper", &adc_fops);
	if (ret < 0){
		return ret;
	}

    adc_device = pdev;

    // Getting resources
	adcmem_rc = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	adcirq_rc = platform_get_resource(pdev, IORESOURCE_IRQ, 0);

	if (!request_mem_region((unsigned long) adcmem_rc->start, adcmem_rc->end - adcmem_rc->start + 1, "temper")) return -EBUSY;
	adcio = ioremap(adcmem_rc->start, adcmem_rc->end - adcmem_rc->start);

	// Enable MCK to ADC
	at91_sys_write(AT91_PMC_PCER, 1 << adcirq_rc->start);

	// Register IRQ
	ret = request_irq(adcirq_rc->start, get_temper_tc1046, 0, "temper", pdev);

	// Control of MCK to ADC
	ret = at91_sys_read(AT91_PMC_PCSR);

	// *** ADC Setup, channel 2, 10 bit etc ***
	writel(AT91C_ADC_TRGEN_DIS |
			AT91C_ADC_LOWRES_10_BIT |
			AT91C_ADC_SLEEP_NORMAL_MODE |
			(63 << 8) | (9 << 16) | (0xF << 24), adcio + ADC_MR);
	// Channel enable
	writel(AT91C_ADC_CH2, adcio + ADC_CHER);
	// Interrupt enable
	writel(AT91C_ADC_EOC2, adcio + ADC_IER);

#ifdef DEBUG
	printk(KERN_INFO "dc1046: irq = %d\n", adcirq_rc->start);
	printk(KERN_INFO "dc1046: phys mem = 0x%X\n", adcmem_rc->start);
	printk(KERN_INFO "dc1046: io mem = 0x%X\n", adcio);
	ret = readl(adcio + ADC_MR);
	printk(KERN_INFO "dc1046: adc mode = 0x%X\n", ret);
	ret = readl(adcio + ADC_IMR);
	printk(KERN_INFO "dc1046: adc int set = 0x%X\n", ret);
	ret = readl(adcio + ADC_CHSR);
	printk(KERN_INFO "dc1046: adc channel status = 0x%X\n", ret);
	ret = readl(adcio + ADC_SR);
	printk(KERN_INFO "dc1046: adc status = 0x%X\n", ret);
#endif

	// Start first conversion
	writel(AT91C_ADC_START, adcio + ADC_CR);

	return 0;
}

static int __exit adc_remove(struct platform_device *pdev)
{

	if (platform_get_drvdata(pdev)) {
		printk(KERN_INFO "dc1046: driver removed. OK.\n");
	}else printk(KERN_INFO "dc1046: driver don't removed. False.\n");

	return 0;
}

static struct platform_driver adc_driver = {
	.remove = __exit_p(adc_remove),

	.driver = {
		.name = "adc",
		.owner = THIS_MODULE,
	},
};


static int __init tc1046_init(void)
{
	int ret;

#ifdef DEBUG
	printk(KERN_INFO "temper init\n");
#endif

	ret = platform_driver_probe(&adc_driver, adc_probe);

	if (ret) {
		// В случае когда девайс еще не добавлен
		platform_driver_unregister(&adc_driver);
		return -ENODEV;
	}

	return 0;
}

static void __exit tc1046_exit(void)
{
	unregister_chrdev(nmajor, "temper");
	platform_driver_unregister(&adc_driver);

#ifdef DEBUG
	printk(KERN_INFO "device_closed\n");
#endif
}

module_init(tc1046_init);
module_exit(tc1046_exit);

MODULE_AUTHOR("Alex AVAlon");
MODULE_SUPPORTED_DEVICE("temper");
MODULE_LICENSE("GPL");
