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
#include <linux/dma-mapping.h>
#include <linux/fb.h>
#include <asm/uaccess.h>

#include <linux/gpio.h>
#include <mach/gpio.h>

#include "am160160_drv.h"
#include "AT91SAM9260_inc.h"

#undef CONFIG_PCI
#undef CONFIG_PM

#define UC1698CMD		0x10000000
#define UC1698DATA		(0x10000000 | (1 << 19))


#define PIXMAP_SIZE	1
#define BUFLEN		160*160*8

unsigned char video[BUFLEN];
unsigned char *pVideo;

/*static struct map_desc uc1698_io_desc[] __initdata = {
	{
		.virtual	= AT91_IO_P2V(UC1698CMD),
		.pfn		= __phys_to_pfn(UC1698CMD),
		.length		= SZ_1K,
		.type		= MT_DEVICE,
	},
	{
		.virtual	= AT91_IO_P2V(UC1698DATA),
		.pfn		= __phys_to_pfn(UC1698DATA),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}
};*/

static struct resource uc1698_resources[]={
		[0]={
			.start	= UC1698CMD,
			.end 	= UC1698CMD + 1,
			.flags 	= IORESOURCE_MEM,
		},
		[1] = {
			.start 	= UC1698DATA,
			.end 	= UC1698DATA + BUFLEN,
			.flags 	= IORESOURCE_MEM,
		},
};

static struct platform_device uc1698_device = {
		.name	= "uc6198",
		.id 	= 0,
		.num_resources	= ARRAY_SIZE(uc1698_resources),
		.resource		= uc1698_resources,
};

static struct platform_device *devices[] __initdata = {
		&uc1698_device,
};

/*
 * Driver data
 */
//static char *mode_option __devinitdata;
struct uc1698_par;
static unsigned char *uc1698_cmd = (unsigned char *) UC1698CMD;
static unsigned char *uc1698_data = (unsigned char *) UC1698DATA;

static int device_cnt=0;

static struct fb_fix_screeninfo uc1698_fb_fix __devinitdata = {
	.id =		"uc1698_fb",
//	.type =		FB_TYPE_PACKED_PIXELS,
	.type =		FB_TYPE_TEXT,
	.visual =	FB_VISUAL_MONO01,
	.xpanstep =	1,
	.ypanstep =	1,
	.ywrapstep =	1, 
	.accel =	FB_ACCEL_NONE,
};

//static struct fb_info info;
//static struct uc1698__par __initdata current_par;

//int uc1698_fb_init(void);

static int uc1698_fb_open(struct fb_info *info, int user)
{
	printk(KERN_INFO "fb_open(%d)\n",user);

    return 0;
}

// (struct fb_info *info, char __user *buf,  size_t count, loff_t *ppos);
static ssize_t uc1698_fb_read(struct fb_info *info, char __user *buffer, size_t length, loff_t *offset)
{
	int i=0;

	printk(KERN_INFO "fb_read(%p,%p,%d)\n",info,buffer,length);

//	for(i=0; i<length && i < BUFLEN; i++) get_user(Video[i], buffer + i);
//	pVideo = Video;

	return i;
}

static ssize_t uc1698_fb_write(struct fb_info *info, const char __user *buffer, size_t length, loff_t *offset)
{
    int rlen = info->fix.smem_len;
    unsigned long pos = (unsigned long) offset;

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
    if (copy_from_user((char *) info->fix.smem_start, (const char __user *) buffer, length)) return -EFAULT;

    // Передача экрана на индикатор


	return length;
}

static int uc1698_fb_release(struct fb_info *info, int user)
{
	printk(KERN_INFO "fb_release(%d)\n",user);

	return 0;
}

//static int uc1698_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
//{
//    /* ... */
//    return 0;	   	
//}
//
//static int uc1698_fb_set_par(struct fb_info *info)
//{
//    struct uc1698__par *par = info->par;
//    /* ... */
//    return 0;	
//}
//
//static int uc1698_fb_setcolreg(unsigned regno, unsigned red, unsigned green,
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
//static int uc1698_fb_pan_display(struct fb_var_screeninfo *var,
//			     struct fb_info *info)
//{
//    return 0;
//}
//
//static int uc1698_fb_blank(int blank_mode, struct fb_info *info)
//{
//    /* ... */
//    return 0;
//}

/* ------------ Accelerated Functions --------------------- */
//void uc1698_fb_fillrect(struct fb_info *p, const struct fb_fillrect *region)
//{
//}
//
//void uc1698_fb_copyarea(struct fb_info *p, const struct fb_copyarea *area) 
//{
//}
//
//
//void uc1698_fb_imageblit(struct fb_info *p, const struct fb_image *image) 
//{
//}
//
//int uc1698_fb_cursor(struct fb_info *info, struct fb_cursor *cursor)
//{
//}
//void uc1698_fb_rotate(struct fb_info *info, int angle)
//{
//}
//
//int uc1698_fb_sync(struct fb_info *info)
//{
//	return 0;
//}

    /*
     *  Frame buffer operations
     */

static struct fb_ops uc1698_fb_ops = {
	.owner		= THIS_MODULE,
	.fb_open	= uc1698_fb_open,
	.fb_read	= uc1698_fb_read,
	.fb_write	= uc1698_fb_write,
	.fb_release	= uc1698_fb_release,
//	.fb_check_var	= uc1698_fb_check_var,
//	.fb_set_par	= uc1698_fb_set_par,
//	.fb_setcolreg	= uc1698_fb_setcolreg,
//	.fb_blank	= uc1698_fb_blank,
//	.fb_pan_display	= uc1698_fb_pan_display,
	.fb_fillrect	= cfb_fillrect, 	/* Needed !!! */
	.fb_copyarea	= cfb_copyarea,	/* Needed !!! */
	.fb_imageblit	= cfb_imageblit,	/* Needed !!! */
//	.fb_cursor	= uc1698_fb_cursor,		/* Optional !!! */
//	.fb_rotate	= uc1698_fb_rotate,
//	.fb_sync	= uc1698_fb_sync,
//	.fb_ioctl	= uc1698_fb_ioctl,
//	.fb_mmap	= uc1698_fb_mmap,
};
//EXPORT_SYMBOL_GPL(uc1698_fb_read);
//EXPORT_SYMBOL_GPL(uc1698_fb_write);

/* ------------------------------------------------------------------------- */

    /*
     *  Initialization
     */
/*static int __devinit uc1698_fb_probe(struct pci_dev *dev,
			      const struct pci_device_id *ent)*/

static int uc1698_fb_probe (struct platform_device *pdev)	// -- for platform devs
{
    struct device *dev = &pdev->dev;
    struct fb_info *info;
    struct uc1698_info *sinfo;
//    struct uc1698_info *pd_sinfo;
//    struct resource *map = NULL;
      struct resource *regs = NULL;
//    struct fb_var_screeninfo *var;
    int cmap_len, retval;	
//    unsigned int smem_len;
    unsigned char *regcmd;


    memset(video, 0, BUFLEN);
    /*
     * Dynamically allocate info and par
     */
    /*
     * Dynamically allocate info and par
     */
    info = framebuffer_alloc(sizeof(struct uc1698_info), dev);
    if (!info) {
	    /* goto error path */
    	return -ENOMEM;
    }
    sinfo = info->par;
    sinfo->info = info;
    sinfo->pdev = pdev;
    info->fbops = &uc1698_fb_ops;

//    iotable_init(uc1698_io_desc, ARRAY_SIZE(uc1698_io_desc));

    uc1698_fb_fix.mmio_start = ioremap(uc1698_data, BUFLEN >> 2);
    uc1698_fb_fix.mmio_len = BUFLEN >> 2;
    regcmd = ioremap(uc1698_cmd, 1);

    dev_err(dev, "resource unusable\n");
    //    if (!mode_option)
    //	mode_option = "160x160@60";

//    retval = fb_find_mode(&info->var, info, NULL, NULL, 0, NULL, 8);
//    if (!retval || retval == 4) 	return -ENOMEM;
// var = uc1698_fb_default;

    uc1698_fb_fix.smem_start = (unsigned long) video;
    uc1698_fb_fix.smem_len = BUFLEN >> 2;
    info->fix = uc1698_fb_fix; /* this will be the only time uc1698_fb_fix will be
     	 	 	 	 	 	 	 * used, so mark it as __devinitdata
     	 	 	 	 	 	 	 */

    printk(KERN_INFO "set i/o sram: %lX+%lX; %lX+%lX\n", uc1698_fb_fix.mmio_start, uc1698_fb_fix.mmio_len, uc1698_fb_fix.smem_start, uc1698_fb_fix.smem_len);

//    info->pseudo_palette = info->par;
//    info->par = NULL;
//    info->flags = FBINFO_FLAG_DEFAULT;

    ((unsigned char *) uc1698_fb_fix.mmio_start)[0] = 0;

/*    cmap_len = 16;
    if (fb_alloc_cmap(&info->cmap, cmap_len, 0) < 0) return -ENOMEM;

    if (register_framebuffer(info) < 0) {
    	fb_dealloc_cmap(&info->cmap);
    	return -EINVAL;
    }

    platform_set_drvdata(pdev, info);*/

    printk(KERN_INFO "fb%d: %s frame buffer device\n", info->node, info->fix.id);

    // Хардварная инициализация индикатора

    return 0;
}

//    /*
//     *  Cleanup
//     */

 //static void __devexit uc1698_fb_remove(struct pci_dev *dev)
static int __exit uc1698_fb_remove(struct platform_device *pdev)
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
 *	uc1698_fb_suspend - Optional but recommended function. Suspend the device.
 *	@dev: platform device
 *	@msg: the suspend event code.
 *
 *      See Documentation/power/devices.txt for more information
 */
static int uc1698_fb_suspend(struct platform_device *dev, pm_message_t msg)
{
	struct fb_info *info = platform_get_drvdata(dev);
	struct uc1698_fb_par *par = info->par;

	/* suspend here */
	return 0;
}

/**
 *	uc1698_fb_resume - Optional but recommended function. Resume the device.
 *	@dev: platform device
 *
 *      See Documentation/power/devices.txt for more information
 */
static int uc1698_fb_resume(struct platform_dev *dev)
{
	struct fb_info *info = platform_get_drvdata(dev);
	struct uc1698_fb_par *par = info->par;

	/* resume here */
	return 0;
}
#else
#define uc1698_fb_suspend NULL
#define uc1698_fb_resume NULL
#endif /* CONFIG_PM */

//static struct platform_device_driver uc1698_fb_driver = {
static struct platform_driver uc1698_fb_driver = {
//	.probe = uc1698_fb_probe,
	.remove = __exit_p(uc1698_fb_remove),

#ifdef CONFIG_PM
	.suspend = uc1698_fb_suspend, /* optional but recommended */
	.resume = uc1698_fb_resume,   /* optional but recommended */
#endif /* CONFIG_PM */

	.driver = {
		.name = "uc1698_fb",
		.owner = THIS_MODULE,
	},
};

static struct platform_device *uc1698_fb_device;

#ifndef MODULE
    /*
     *  Setup
     */

/*
 * Only necessary if your driver takes special options,
 * otherwise we fall back on the generic fb_setup().
 */
int __init uc1698_fb_setup(char *options)
{
    /* Parse user speficied options (`video=uc1698_fb:') */
}
#endif /* MODULE */

static int __init uc1698_fb_init(void)
{
	int ret;
	/*
	 *  For kernel boot options (in 'video=uc1698_fb:<options>' format)
	 */
	
    if (device_cnt++) return -EINVAL;

#ifndef MODULE
	char *option = NULL;

	if (fb_get_options("uc1698_fb", &option))
		return -ENODEV;
	uc1698_fb_setup(option);
#endif

	at91_set_GPIO_periph(AT91_PIN_PC4, 0);
	at91_set_GPIO_periph(AT91_PIN_PC5, 0);
	at91_set_GPIO_periph(AT91_PIN_PC6, 0);
	at91_set_GPIO_periph(AT91_PIN_PC7, 0);
	at91_set_gpio_output(AT91_PIN_PC4, 1);
	at91_set_gpio_output(AT91_PIN_PC5, 1);
	at91_set_gpio_output(AT91_PIN_PC6, 0);
	at91_set_gpio_output(AT91_PIN_PC7, 0);

	ret = platform_add_devices(devices, ARRAY_SIZE(devices));

	if (ret){

		printk(KERN_INFO "no add device\n");

	}

	ret = platform_driver_register(&uc1698_fb_driver);

	if (!ret) {
//		uc1698_fb_device = platform_device_register_simple("uc1698_fb", 0,
//								NULL, 0);

		uc1698_fb_device = platform_device_alloc("uc1698_fb", 0);
		if (uc1698_fb_device)
			ret = platform_device_add(uc1698_fb_device);
		else
			ret = -ENOMEM;

		if (ret) {
			platform_device_put(uc1698_fb_device);
			platform_driver_unregister(&uc1698_fb_driver);
		}else ret = uc1698_fb_probe(uc1698_fb_device);
	}

	printk(KERN_INFO "device_open(%d)\n",ret);

	return ret;
}

static void __exit uc1698_fb_exit(void)
{
	device_cnt--;
	platform_device_unregister(uc1698_fb_device);
	platform_driver_unregister(&uc1698_fb_driver);
}

/* ------------------------------------------------------------------------- */


    /*
     *  Modularization
     */

module_init(uc1698_fb_init);
module_exit(uc1698_fb_exit);

MODULE_LICENSE("GPL");
