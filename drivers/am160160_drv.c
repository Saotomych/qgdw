/*
 * linux/drivers/video/skeletonfb.c -- Skeleton for a frame buffer device
 *
 *  Modified to new api Jan 2001 by James Simmons (jsimmons@transvirtual.com)
 *
 *  Created 28 Dec 1997 by Geert Uytterhoeven
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/backlight.h>
#include <linux/platform_device.h>
//#include <linux/dma-mapping.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>

#include <mach/at91sam9_smc.h>

#include "am160160_drv.h"
#include "lcdfuncs.h"

#undef CONFIG_PCI
#undef CONFIG_PM

#define am160160CMD			AT91_CHIPSELECT_0	// 0x10000000
#define am160160DATA		(AT91_CHIPSELECT_0 | (1 << 19))
#define am160160CMDWR		(AT91_CHIPSELECT_0 | (1 << 20))
#define am160160DATAWR		(AT91_CHIPSELECT_0 | (1 << 19) | (1 <<20))
#define resetpin		AT91_PIN_PC4
#define lightpin		AT91_PIN_PC5

#define PIXMAP_SIZE	1
#define BUF_LEN		80*160

static unsigned char video[BUF_LEN];
static PAMLCDFUNC hard;
unsigned char *io_cmd, *io_data, *io_cmdwr, *io_datawr;								// virtual i/o indicator addresses

/* Board depend */
static struct resource am160160_resources[]={
		[0]={
			.start	= am160160CMD,
			.end 	= am160160CMD + 1,
			.flags 	= IORESOURCE_MEM,
		},
		[1] = {
			.start 	= am160160DATA,
			.end 	= am160160DATA + BUF_LEN,
			.flags 	= IORESOURCE_MEM,
		},
		[2] = {
			.start 	= am160160CMDWR,
			.end 	= am160160CMDWR + 1,
			.flags 	= IORESOURCE_MEM,
		},
		[3] = {
			.start 	= am160160DATAWR,
			.end 	= am160160DATAWR + BUF_LEN,
			.flags 	= IORESOURCE_MEM,
		},
};

static struct platform_device am160160_device = {
		.name	= "am160160",
		.id 	= 0,
		.num_resources	= ARRAY_SIZE(am160160_resources),
		.resource		= am160160_resources,
};

static struct platform_device *devices[] __initdata = {
		&am160160_device,
};
/* End Board depend */


/*
 * Driver data
 */
static char *mode_option __initdata;
struct am160160_par;
static unsigned char *am160160_cmd = (unsigned char *) am160160CMD;		// phys i/o indicator addresses
static unsigned char *am160160_data = (unsigned char *) am160160DATA;

static int device_cnt=0;

static struct fb_fix_screeninfo am160160_fb_fix __devinitdata = {
	.id =		"am160160_fb",
//	.type =		FB_TYPE_PACKED_PIXELS,
	.type =		FB_TYPE_TEXT,
	.visual =	FB_VISUAL_MONO01,
	.xpanstep =	1,
	.ypanstep =	1,
	.ywrapstep =	1, 
	.accel =	FB_ACCEL_NONE,
};

//static struct fb_info info;
//static struct am160160__par __initdata current_par;

//int am160160_fb_init(void);

static int am160160_fb_open(struct fb_info *info, int user)
{
	printk(KERN_INFO "fb_open(%d)\n",user);

    return 0;
}

// (struct fb_info *info, char __user *buf,  size_t count, loff_t *ppos);
static ssize_t am160160_fb_read(struct fb_info *info, char __user *buffer, size_t length, loff_t *offset)
{
	unsigned int len=0, i;
	unsigned char *prddata = 0;

	len = hard->readdata(prddata, BUF_LEN);
	if ((prddata) && (len)){
		for(i=0; i<length && i < info->fix.smem_len; i++) put_user(prddata[i], (char __user *) buffer + i);
		printk(KERN_INFO "fb_read(%p,%p,%d)\n",info,buffer,length);
	}

	return len;
}

static ssize_t am160160_fb_write(struct fb_info *info, const char __user *buffer, size_t length, loff_t *offset)
{
    size_t rlen = info->fix.smem_len;
//    unsigned long pos = (unsigned long) offset;
    char *pvideo = video;

	printk(KERN_INFO "fb_write(%p,%p,%d,%lX)\n",info,buffer,length, (long unsigned int)offset);

	// Вместо офсета в реале приходит какая-то эпическая хуйня, поэтому пока не юзаем
	// Выяснить где и как оно выставляется и юзается.

	// Проверка на переход смещения за границу буфера и на приход блока с длиной 0
	//    if (pos >= info->fix.smem_len || (!length)) return length;
	if (!length) return 0;

    // Если пришедший блок короче, чем выделенная память, то ставим длину этого блока.
	// Обычно ожидается приход блока равный памяти
    if (length <= info->fix.smem_len) rlen = length;
    // Если текущая длина + смешщение больше, чем выделенная память, то вычитаем смещение
    //    if (rlen + pos > info->fix.smem_len) rlen-=pos;

    // Читаем в буфер блок данных
    if (copy_from_user(pvideo, (const char __user *) buffer, rlen)) return -EFAULT;

    printk(KERN_INFO "write %x %x %x %x %x %x %x %x %x %x \n length %d \n", pvideo[0],  pvideo[1],  pvideo[2],  pvideo[3],
    		 pvideo[4],  pvideo[5],  pvideo[6],  pvideo[8],  pvideo[9],  pvideo[10], rlen);

    hard->writedat(pvideo, rlen);

//    pvideo[rlen - 1] = 0;
    printk(KERN_INFO "read %x %x %x %x %x %x %x %x %x %x \n length %d \n", pvideo[0],  pvideo[1],  pvideo[2],  pvideo[3],
    		 pvideo[4],  pvideo[5],  pvideo[6],  pvideo[8],  pvideo[9],  pvideo[10], rlen);

	return rlen;
}

static int am160160_fb_release(struct fb_info *info, int user)
{
	printk(KERN_INFO "fb_release(%d)\n",user);

	return 0;
}

//static int am160160_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
//{
//    /* ... */
//    return 0;	   	
//}
//
//static int am160160_fb_set_par(struct fb_info *info)
//{
//    struct am160160__par *par = info->par;
//    /* ... */
//    return 0;	
//}
//
//static int am160160_fb_setcolreg(unsigned regno, unsigned red, unsigned green,
//			   unsigned blue, unsigned transp,
//			   struct fb_info *info)
//{
//    if (regno >= 256)  /* no. of hw registers */
//       return -EINVAL;
//    /*
//     * Program hardware... do anything you want with transp
//     */
//
//    /* grayscale works only partially under directcolor */
//    if (info->var.grayscale) {
//       /* grayscale = 0.30*R + 0.59*G + 0.11*B */
//       red = green = blue = (red * 77 + green * 151 + blue * 28) >> 8;
//    }
//
//    // Mono01/Mono10:
//    // Has only 2 values, black on white or white on black (fg on bg),
//    // var->{color}.offset is 0
//    /// white = (1 << var->{color}.length) - 1, black = 0
//    
//#define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)
//    red = CNVT_TOHW(red, info->var.red.length);
//    green = CNVT_TOHW(green, info->var.green.length);
//    blue = CNVT_TOHW(blue, info->var.blue.length);
//    transp = CNVT_TOHW(transp, info->var.transp.length);
//#undef CNVT_TOHW
//
//    if (info->fix.visual == FB_VISUAL_DIRECTCOLOR ||
//	info->fix.visual == FB_VISUAL_TRUECOLOR)
//	    write_{red|green|blue|transp}_to_clut();
//
//    if (info->fix.visual == FB_VISUAL_TRUECOLOR ||
//	info->fix.visual == FB_VISUAL_DIRECTCOLOR) {
//	    u32 v;
//
//	    if (regno >= 16)
//		    return -EINVAL;
//
//	    v = (red << info->var.red.offset) |
//		    (green << info->var.green.offset) |
//		    (blue << info->var.blue.offset) |
//		    (transp << info->var.transp.offset);
//
//	    ((u32*)(info->pseudo_palette))[regno] = v;
//    }
//
//    /* ... */
//    return 0;
//}
//
//static int am160160_fb_pan_display(struct fb_var_screeninfo *var,
//			     struct fb_info *info)
//{
//    return 0;
//}
//
//static int am160160_fb_blank(int blank_mode, struct fb_info *info)
//{
//    /* ... */
//    return 0;
//}

/* ------------ Accelerated Functions --------------------- */
//void am160160_fb_fillrect(struct fb_info *p, const struct fb_fillrect *region)
//{
//}
//
//void am160160_fb_copyarea(struct fb_info *p, const struct fb_copyarea *area)
//{
//}
//
//
//void am160160_fb_imageblit(struct fb_info *p, const struct fb_image *image)
//{
//}
//
//int am160160_fb_cursor(struct fb_info *info, struct fb_cursor *cursor)
//{
//}
//void am160160_fb_rotate(struct fb_info *info, int angle)
//{
//}
//
//int am160160_fb_sync(struct fb_info *info)
//{
//	return 0;
//}

    /*
     *  Frame buffer operations
     */

static struct fb_ops am160160_fb_ops = {
	.owner		= THIS_MODULE,
	.fb_open	= am160160_fb_open,
	.fb_read	= am160160_fb_read,
	.fb_write	= am160160_fb_write,
	.fb_release	= am160160_fb_release,
//	.fb_check_var	= am160160_fb_check_var,
//	.fb_set_par	= am160160_fb_set_par,
//	.fb_setcolreg	= am160160_fb_setcolreg,
//	.fb_blank	= am160160_fb_blank,
//	.fb_pan_display	= am160160_fb_pan_display,
	.fb_fillrect	= cfb_fillrect, 	/* Needed !!! */
	.fb_copyarea	= cfb_copyarea,	/* Needed !!! */
	.fb_imageblit	= cfb_imageblit,	/* Needed !!! */
//	.fb_cursor	= am160160_fb_cursor,		/* Optional !!! */
//	.fb_rotate	= am160160_fb_rotate,
//	.fb_sync	= am160160_fb_sync,
//	.fb_ioctl	= am160160_fb_ioctl,
//	.fb_mmap	= am160160_fb_mmap,
};
//EXPORT_SYMBOL_GPL(am160160_fb_read);
//EXPORT_SYMBOL_GPL(am160160_fb_write);

/* ------------------------------------------------------------------------- */

    /*
     *  Initialization
     */
/*static int __devinit am160160_fb_probe(struct pci_dev *dev,
			      const struct pci_device_id *ent)*/

static int am160160_fb_probe (struct platform_device *pdev)	// -- for platform devs
{
    struct device *dev = &pdev->dev;
    struct fb_info *info;
    unsigned char *pinfo;
//    struct am160160_info *sinfo;
//    struct am160160_info *pd_sinfo;
//    struct resource *map = NULL;
//    struct fb_var_screeninfo *var;
    int cmap_len, retval;	
//    unsigned int smem_len;

    memset(video, 0, BUF_LEN);
    /*
     * Dynamically allocate info and par
     */
    info = framebuffer_alloc(sizeof(u32) * BUF_LEN, dev);
//    info = framebuffer_alloc(sizeof(struct uc1698_info), dev);
    if (!info) {
	    /* goto error path */
    	return -ENOMEM;
    }

    info->screen_base = (char __iomem *) io_data;
    info->fbops = &am160160_fb_ops;

	mode_option = "160x160@60";

    retval = fb_find_mode(&info->var, info, mode_option, NULL, 0, NULL, 8);
    if (!retval || retval == 4) 	return -EINVAL;
//    info->var =

    am160160_fb_fix.smem_start = (unsigned long) video;
    am160160_fb_fix.smem_len = BUF_LEN;
    am160160_fb_fix.mmio_start = io_data;
    am160160_fb_fix.mmio_len = BUF_LEN;
    info->fix = am160160_fb_fix; /* this will be the only time am160160_fb_fix will be
     	 	 	 	 	 	 	 * used, so mark it as __devinitdata
     	 	 	 	 	 	 	 */

    info->pseudo_palette = info->par;
    info->par = NULL;
    info->flags = FBINFO_FLAG_DEFAULT;

    printk(KERN_INFO "set i/o sram: %lX+%lX; %lX+%lX\n", am160160_fb_fix.mmio_start, am160160_fb_fix.mmio_len, am160160_fb_fix.smem_start, am160160_fb_fix.smem_len);

    cmap_len = 16;
    if (fb_alloc_cmap(&info->cmap, cmap_len, 0) < 0) return -ENOMEM;

    printk(KERN_INFO "fb%d: %s frame buffer device\n", info->node, info->fix.id);

    if (register_framebuffer(info) < 0) {
    	fb_dealloc_cmap(&info->cmap);
    	return -EINVAL;
    }

    platform_set_drvdata(pdev, info);

    printk(KERN_INFO "fb%d: %s frame buffer device\n", info->node, info->fix.id);

    pinfo = hard->readinfo();
    printk(KERN_INFO "rd 0x%X 0x%X 0x%X", pinfo[0],pinfo[1],pinfo[2]);
    return 0;
}

//    /*
//     *  Cleanup
//     */

 //static void __devexit am160160_fb_remove(struct pci_dev *dev)
static int __exit am160160_fb_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);
	/* or platform_get_drvdata(pdev); */

	if (info) {
		unregister_framebuffer(info);
		fb_dealloc_cmap(&info->cmap);
		/* ... */
		framebuffer_release(info);
	}
	
	return 0;
	
}

/* for platform devices */

#ifdef CONFIG_PM
/**
 *	am160160_fb_suspend - Optional but recommended function. Suspend the device.
 *	@dev: platform device
 *	@msg: the suspend event code.
 *
 *      See Documentation/power/devices.txt for more information
 */
static int am160160_fb_suspend(struct platform_device *dev, pm_message_t msg)
{
	struct fb_info *info = platform_get_drvdata(dev);
	struct am160160_fb_par *par = info->par;

	/* suspend here */
	return 0;
}

/**
 *	am160160_fb_resume - Optional but recommended function. Resume the device.
 *	@dev: platform device
 *
 *      See Documentation/power/devices.txt for more information
 */
static int am160160_fb_resume(struct platform_dev *dev)
{
	struct fb_info *info = platform_get_drvdata(dev);
	struct am160160_fb_par *par = info->par;

	/* resume here */
	return 0;
}
#else
#define am160160_fb_suspend NULL
#define am160160_fb_resume NULL
#endif /* CONFIG_PM */

//static struct platform_device_driver am160160_fb_driver = {
static struct platform_driver am160160_fb_driver = {
//	.probe = am160160_fb_probe,
	.remove = __exit_p(am160160_fb_remove),

#ifdef CONFIG_PM
	.suspend = am160160_fb_suspend, /* optional but recommended */
	.resume = am160160_fb_resume,   /* optional but recommended */
#endif /* CONFIG_PM */

	.driver = {
		.name = "am160160_fb",
		.owner = THIS_MODULE,
	},
};

static struct platform_device *am160160_fb_device;

#ifndef MODULE
    /*
     *  Setup
     */

/*
 * Only necessary if your driver takes special options,
 * otherwise we fall back on the generic fb_setup().
 */
int __init am160160_fb_setup(char *options)
{
    /* Parse user speficied options (`video=am160160_fb:') */
}
#endif /* MODULE */

static int __init am160160_fb_init(void)
{
	int ret;
	unsigned char *pinfo;
	/*
	 *  For kernel boot options (in 'video=am160160_fb:<options>' format)
	 */

	/* Platform & board depend */
	/* SMC SRAM Initialise: Byte write access type, NCS0, WR, RD */
	at91_sys_write(AT91_SMC_SETUP(0), AT91_SMC_NWESETUP_(1) | AT91_SMC_NCS_WRSETUP_(0) | AT91_SMC_NRDSETUP_(2) | AT91_SMC_NCS_RDSETUP_(0));
	at91_sys_write(AT91_SMC_PULSE(0), AT91_SMC_NWEPULSE_(13) | AT91_SMC_NCS_WRPULSE_(16) | AT91_SMC_NRDPULSE_(4) | AT91_SMC_NCS_RDPULSE_(8));
	at91_sys_write(AT91_SMC_CYCLE(0), AT91_SMC_NWECYCLE_(18) | AT91_SMC_NRDCYCLE_(18));
	at91_sys_write(AT91_SMC_MODE(0), AT91_SMC_DBW_8 | AT91_SMC_WRITEMODE | AT91_SMC_READMODE | AT91_SMC_TDF_(1) | AT91_SMC_EXNWMODE_DISABLE);

	/* Pins initialize */
	at91_set_GPIO_periph(resetpin, 0);
	at91_set_gpio_output(resetpin, 1);
	at91_set_GPIO_periph(lightpin, 0);
	at91_set_gpio_output(lightpin, 1);
	/* End platform & board depend */

	// Registration I/O mem for indicator registers
	platform_add_devices(devices, ARRAY_SIZE(devices));
	io_data = ioremap(am160160_data, BUF_LEN);
	io_cmd = ioremap(am160160_cmd, 1);
	io_datawr = ioremap(am160160_data, BUF_LEN);
	io_cmdwr = ioremap(am160160_cmd, 1);

	/* Hardware initialize and testing */
	// Connect to hardware driver
 	hard = uc1698_connect(io_cmd, io_data);
//	hard = st7529_connect(io_cmd, io_data);
	hard->init();
	pinfo = hard->readinfo();

    if (device_cnt++) return -EINVAL;

#ifndef MODULE
	char *option = NULL;

	if (fb_get_options("am160160_fb", &option))
		return -ENODEV;
	am160160_fb_setup(option);
#endif

	ret = platform_driver_probe(&am160160_fb_driver, am160160_fb_probe);
	if (ret) {
		// В случае когда девайс еще не добавлен
		am160160_fb_device = platform_device_alloc("am160160_fb", 0);
		if (am160160_fb_device)
			ret = platform_device_add(am160160_fb_device);
		else{
			device_cnt--;
			ret = -ENOMEM;
		}
		if (ret) {
			platform_device_put(am160160_fb_device);
			platform_driver_unregister(&am160160_fb_driver);
		}else ret = platform_driver_probe(&am160160_fb_driver, am160160_fb_probe); //am160160_fb_probe(am160160_fb_device);
	}

	printk(KERN_INFO "device_open(%d)\n",ret);

	return ret;
}

static void __exit am160160_fb_exit(void)
{
	device_cnt--;
	platform_driver_unregister(&am160160_fb_driver);
	platform_device_unregister(am160160_fb_device);
	platform_device_del(&am160160_device);
	hard->exit();
	printk(KERN_INFO "device_close(%d)\n",ret);
}

/* ------------------------------------------------------------------------- */


    /*
     *  Modularization
     */

module_init(am160160_fb_init);
module_exit(am160160_fb_exit);

MODULE_LICENSE("GPL");
