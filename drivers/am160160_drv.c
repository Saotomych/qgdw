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

#include "am160160_drv.h"

#undef CONFIG_PCI
#undef CONFIG_PM

#define PIXMAP_SIZE	1
#define BUF_LEN		160*160*8

unsigned char video[BUF_LEN];
unsigned char *pVideo;

/*
 * Driver data
 */
static char *mode_option __devinitdata;
struct uc1698_par;

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

//	for(i=0; i<length && i < BUF_LEN; i++) get_user(Video[i], buffer + i);
//	pVideo = Video;

	return i;
}

static ssize_t uc1698_fb_write(struct fb_info *info, const char __user *buffer, size_t length, loff_t *offset)
{
    int rlen = info->fix.smem_len;
    unsigned long pos = (unsigned long) offset;

	printk(KERN_INFO "fb_write(%p,%p,%d,%ld)\n",info,buffer,length, (long) offset);

	// Проверка на переход смещения за границу буфера и на приход блока с длиной 0
//    if (pos >= info->fix.smem_len || (!length)) return 0;

    // Если пришедший блок длиннее чем выделенная память, то ставим длину выделенной памяти
//    if (length <= info->fix.smem_len) rlen = length;
    // Если текущая длина + смешщение больше, чем выделенная память, то вычитаем смещение
//    if (rlen + pos > info->fix.smem_len) rlen-=pos;

    // Читаем в буфер блок данных
//    if (copy_from_user((char *) info->fix.smem_start, (const char __user *) buffer, rlen)) return -EFAULT;

//    offset += length;
	offset++;

//    return length;
	return 1;

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
EXPORT_SYMBOL_GPL(uc1698_fb_read);
EXPORT_SYMBOL_GPL(uc1698_fb_write);

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
    struct uc1698_info *pd_sinfo;
    struct resource *map = NULL;
    struct fb_var_screeninfo *var;
    int cmap_len, retval;	
    unsigned int smem_len;
    
    memset(video, 0, BUF_LEN);
    /*
     * Dynamically allocate info and par
     */
    info = framebuffer_alloc(sizeof(u32) * (BUF_LEN>>2), dev);
    if (!info) {
	    /* goto error path */
    }

    info->screen_base = (char __iomem *) video;
    info->fbops = &uc1698_fb_ops;

    //    if (!mode_option)
    //	mode_option = "160x160@60";

    retval = fb_find_mode(&info->var, info, NULL, NULL, 0, NULL, 8);
    if (!retval || retval == 4) 	return -ENOMEM;
// var = uc1698_fb_default;

    uc1698_fb_fix.smem_start = (unsigned long) video;
    uc1698_fb_fix.smem_len = BUF_LEN;
    info->fix = uc1698_fb_fix; /* this will be the only time uc1698_fb_fix will be
     	 	 	 	 	 	 	 * used, so mark it as __devinitdata
     	 	 	 	 	 	 	 */
    info->pseudo_palette = info->par;
    info->par = NULL;
    info->flags = FBINFO_FLAG_DEFAULT;

    cmap_len = 16;
    if (fb_alloc_cmap(&info->cmap, cmap_len, 0) < 0) return -ENOMEM;

    if (register_framebuffer(info) < 0) {
    	fb_dealloc_cmap(&info->cmap);
    	return -EINVAL;
    }

    platform_set_drvdata(pdev, info);


    printk(KERN_INFO "fb%d: %s frame buffer device\n", info->node, info->fix.id);

    return 0;

//    var = &info->var;

//    sinfo = info->par;
//    if (dev->platform_data){
//        pd_sinfo = dev->platform_data;
//
//        sinfo->default_bpp = pd_sinfo->default_bpp;
//    	sinfo->default_dmacon = pd_sinfo->default_dmacon;
//    	sinfo->default_lcdcon2 = pd_sinfo->default_lcdcon2;
//    	sinfo->default_monspecs = pd_sinfo->default_monspecs;
//    	sinfo->u1698_fb_power_control = pd_sinfo->u1698_fb_power_control;
//    	sinfo->guard_time = pd_sinfo->guard_time;
//    	sinfo->smem_len = pd_sinfo->smem_len;
//    	sinfo->lcdcon_is_backlight = pd_sinfo->lcdcon_is_backlight;
//    	sinfo->lcd_wiring_mode = pd_sinfo->lcd_wiring_mode;
//    }
//    sinfo->info = info;
//    sinfo->pdev = pdev;
//    strcpy(info->fix.id, sinfo->pdev->name);

    
    /* 
     * Here we set the screen_base to the virtual memory address
     * for the framebuffer. Usually we obtain the resource address
     * from the bus layer and then translate it to virtual memory
     * space via ioremap. Consult ioport.h. 
     */
//    info->screen_base = framebuffer_virtual_memory;
    /* Set Video Memory */
//    map = platform_get_resource(pdev, IORESOURCE_MEM, 1);
//    if (map){ // Already exist
//    	// use pre-allocated memory buffer
//    	info->fix.smem_start = map->start;
//    	info->fix.smem_len = map->end - map->start + 1;
//
////    	if (!request_mem_region(info->fix.smem.start, info->fix.smem.len)){
//    		// Goto ERROR
////    	}
//
//    	info->screen_base = ioremap(info->fix.smem_start, info->fix.smem_len);
//
//    }else{	// Non exist
//    	smem_len = (var->xres_virtual * var->yres_virtual * ((var->bits_per_pixel + 7) / 8));
//    	info->fix.smem_len = max(smem_len, sinfo->smem_len);
//
////
//    	//info->screen_base =  dma_alloc_writecombine(info->device, info->fix.smem_len, (dma_addr_t *) &info->fix.smem_start, GFP_KERNEL);
//
//    	memset(info->screen_base, 0, info->fix.smem_len);
//    }

    /* 
     * The following is done in the case of having hardware with a static 
     * mode. If we are setting the mode ourselves we don't call this. 
     */	
//    info->var = uc1698_fb_var;

    /*
     * For drivers that can...
     */
//    uc1698_fb_check_var(&info->var, info);

    /*
     * Does a call to fb_set_par() before register_framebuffer needed?  This
     * will depend on you and the hardware.  If you are sure that your driver
     * is the only device in the system, a call to fb_set_par() is safe.
     *
     * Hardware in x86 systems has a VGA core.  Calling set_par() at this
     * point will corrupt the VGA console, so it might be safer to skip a
     * call to set_par here and just allow fbcon to do it for you.
     */
    /* uc1698_fb_set_par(info); */

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
	.probe = uc1698_fb_probe,
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
	
#ifndef MODULE
	char *option = NULL;

	if (fb_get_options("uc1698_fb", &option))
		return -ENODEV;
	uc1698_fb_setup(option);
#endif
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
		}
	}

	printk(KERN_INFO "device_open(%d)\n",ret);

	return ret;
}

static void __exit uc1698_fb_exit(void)
{
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
