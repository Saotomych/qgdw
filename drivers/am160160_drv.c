/*
 * linux/drivers/video/skeletonfb.c -- Skeleton for a frame buffer device
 *
 *  Modified to new api Jan 2001 by James Simmons (jsimmons@transvirtual.com)
 *
 *  Created 28 Dec 1997 by Geert Uytterhoeven
 *
 *  am160160_drv - driver for indicator am160160 Andorin with support fb
 *  Created by 04.04.2011
 *  Author: alex AAV
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
//#include <linux/backlight.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/console.h>
#include <linux/vt.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <mach/at91sam9_smc.h>

#include "am160160_drv.h"
#include "lcdfuncs.h"

#undef CONFIG_PCI
#undef CONFIG_PM

#define PIXMAP_SIZE	1
#define BUF_LEN		80*160

static char defchipname[]={"uc1698"};
static char *chipname = defchipname;
module_param_string(chip, defchipname, 7, 0);

static unsigned char video[BUF_LEN];
static unsigned char tmpvd[BUF_LEN];
static PAMLCDFUNC hard = 0;
unsigned char *io_cmd, *io_data;								// virtual i/o indicator addresses
static struct console *am160160fbcon;

/* Board depend */
static struct resource *am160160_resources[3];
static struct platform_device *am160160_device;
static unsigned int resetpin;
static unsigned int lightpin;

/*
 * Driver data
 */
static char *mode_option __initdata;
struct am160160_par;

static int device_cnt=0;

static struct fb_fix_screeninfo am160160_fb_fix __devinitdata = {
	.id =		"am160160",
	.type =		FB_TYPE_PACKED_PIXELS,
	.visual =	FB_VISUAL_MONO01,
	.xpanstep =	0,
	.ypanstep =	0,
	.ywrapstep = 0,
	.accel =	FB_ACCEL_NONE,
	.line_length = 16,
};

//static struct fb_monspecs am_monspecs = {
//
//};

static struct fb_videomode def_fb_videomode = {
	.name = "160x160-1@6",
	.refresh = 6,
	.xres = 160,
	.yres = 160,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = FB_IGNOREMON,
};

static struct fb_info def_fb_info = {

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
    unsigned int x, i;
    unsigned char mask;

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
    if (copy_from_user(tmpvd, (const char __user *) buffer, rlen)) return -EFAULT;

    // Decode to indicator format from 2color bmp
    memset(video, 0, BUF_LEN);
    for (x=0; x<rlen; x++){
    	mask = 0x80;
    	for (i=0; i<8; i++){
    		if (tmpvd[x] & mask){
    			pvideo[(x<<2)+(i>>1)] |= ((i & 1) ? 0x8 : 0x80);
    		}
    		mask >>= 1;
    	}
    }

    printk(KERN_INFO "write %x %x %x %x %x %x %x %x %x %x \n length %d \n", pvideo[0],  pvideo[1],  pvideo[2],  pvideo[3],
    		 pvideo[4],  pvideo[5],  pvideo[6],  pvideo[8],  pvideo[9],  pvideo[10], rlen);

    hard->writedat(pvideo, rlen << 2);

	return rlen;
}

static int am160160_fb_release(struct fb_info *info, int user)
{
	printk(KERN_INFO "fb_release(%d)\n",user);

	return 0;
}

static void conwrite(struct console *con, const char *text, unsigned int length){

	printk(KERN_INFO "con_write (%d) %c%c%c%c%c%c%c%c%c%c\n",length,text[0],text[1],text[2],text[3],text[4],text[5],text[6],text[7],text[8],text[9]);

	if (!length) return;

//	if (hard) hard->writedat(text, length);

    // Читаем в буфер блок данных
//    if (copy_from_user(tmpvd, (const char __user *) text, length)) return;

}

static void my_imageblit(struct fb_info *pinfo, const struct fb_image *image){
char *pvideo = video;
unsigned int x, i;
unsigned char mask;
//	printk(KERN_INFO "fb_imageblit(%lX), w=%d, h=%d, depth=%d\n", pinfo, image->width, image->height, image->depth);

//	sys_imageblit(pinfo, image);

	memcpy(tmpvd, image->data, BUF_LEN);
	// Decode to indicator format from 2color bmp
    pvideo = video;
    memset(video, 0, BUF_LEN);
    for (x=0; x < 1940; x++){
    	mask = 0x80;
    	for (i=0; i<8; i++){
    		if (tmpvd[x] & mask){
    			pvideo[(x<<2)+(i>>1)] |= ((i & 1) ? 0x8 : 0x80);
    		}
    		mask >>= 1;
    	}
    }

//    printk(KERN_INFO "write %x %x %x %x %x %x %x %x %x %x \n", image->data[0],  image->data[1],  image->data[2],  image->data[3],
//    		 image->data[4],  image->data[5],  image->data[6],  image->data[8],  image->data[9],  image->data[10]);

	if (hard) hard->writedat(pvideo, BUF_LEN);
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
	.fb_fillrect	= sys_fillrect, 	/* Needed !!! */
	.fb_copyarea	= sys_copyarea,	/* Needed !!! */
//	.fb_imageblit	= sys_imageblit,	/* Needed !!! */
	.fb_imageblit	= my_imageblit,	/* Needed !!! */
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
    struct fb_info *info = 0;
    unsigned char *pinfo = 0;
//    struct am160160_info *aminfo;
    struct fb_info *pd_sinfo;
    struct fb_var_screeninfo *var = 0;
    struct fb_videomode *mode = 0;
    int cmap_len, retval;	
    struct list_head *head = 0;
    struct fb_modelist *list = 0;
    struct fb_event event;
    struct fb_con2fbmap con2fb;
//    unsigned int smem_len;

    am160160_device = pdev;

	am160160_resources[0] = platform_get_resource(pdev, IORESOURCE_IO, 0);
	am160160_resources[1] = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	am160160_resources[2] = platform_get_resource(pdev, IORESOURCE_MEM, 1);

	printk(KERN_INFO "cmd cmd 0x%X\n", am160160_resources[1]->start);
	printk(KERN_INFO "cmd dat 0x%X\n", am160160_resources[2]->start);
	printk(KERN_INFO "reset 0x%X light 0x%X\n", am160160_resources[0]->start, am160160_resources[0]->end);
	printk(KERN_INFO "device_open (0x%X) 0x%X 0x%X 0x%X\n", pdev, am160160_resources[0], am160160_resources[1], am160160_resources[2]);

	/* Pins initialize */
	resetpin = am160160_resources[0]->start;
	lightpin = am160160_resources[0]->end;
	at91_set_GPIO_periph(resetpin, 0);
	at91_set_gpio_output(resetpin, 0);
	mdelay(10);
	at91_set_gpio_output(resetpin, 1);
	at91_set_GPIO_periph(lightpin, 0);
	at91_set_gpio_output(lightpin, 1);

// Registration I/O mem for indicator registers
	io_cmd = ioremap(am160160_resources[1]->start, 1);
	io_data = ioremap(am160160_resources[2]->start, BUF_LEN);
	if (!request_mem_region(io_cmd, 1, pdev->name)) return -EBUSY;
	if (!request_mem_region(io_data, BUF_LEN, pdev->name)){
		// exception 1
		release_mem_region(io_cmd, 1);
		return -EBUSY;
	}

	// Устройство описано в файле ядра board-sam9260dpm.c
	/* Hardware initialize and testing */
	// Connect to hardware driver
	if (strstr(chipname,"uc1698")) hard = uc1698_connect(io_cmd, io_data);
	if (strstr(chipname,"st7529")) hard = st7529_connect(io_cmd, io_data);
	hard->init();

    memset(video, 0, BUF_LEN);
    /*
     * Dynamically allocate info and par
     */
    var = kmalloc(sizeof(struct fb_var_screeninfo), GFP_KERNEL);
    if (!var){
    	// exception 2
		release_mem_region(io_data, BUF_LEN);
		release_mem_region(io_cmd, 1);
    	return -ENOMEM;
    }

    mode = kmalloc(sizeof(struct fb_videomode), GFP_KERNEL);
    if (!mode){
    	// exception 3
    	kfree(var);
		release_mem_region(io_data, BUF_LEN);
		release_mem_region(io_cmd, 1);
    	return -ENOMEM;
    }
    memcpy(mode, &def_fb_videomode, sizeof(struct fb_videomode));

    info = framebuffer_alloc(sizeof(u32) * BUF_LEN, dev);
    if (!info){
    	// exception 4
    	kfree(mode);
    	kfree(var);
    	release_mem_region(io_data, BUF_LEN);
		release_mem_region(io_cmd, 1);
    	return -ENOMEM;
    }

    if (dev->platform_data){
    	pd_sinfo = (struct fb_info *) dev->platform_data;
    	printk(KERN_INFO "Platform data exist\n");
    }else{
    	pd_sinfo = &def_fb_info;
    	printk(KERN_INFO "Platform data not exist, will default data\n");
    }

    memcpy(info, pd_sinfo, sizeof(struct fb_info));			// copy default fb_info to working fb_info

    mode = &def_fb_videomode;
    fb_videomode_to_var(var, mode);
    // Change videomode
    var->bits_per_pixel = 1;
    var->xres = 160;
    var->yres = 160;
    var->xres_virtual = 160;
    var->yres_virtual = 160;
    var->grayscale = 1;
    var->red.length = 1;
    var->green.length = 1;
    var->blue.length = 1;
    var->red.msb_right =1;
    var->green.msb_right =1;
    var->blue.msb_right =1;
    memcpy(&(info->var), var, sizeof(struct fb_var_screeninfo));

    info->fbops = &am160160_fb_ops;
    info->fix = am160160_fb_fix; /* this will be the only time am160160_fb_fix will be
     	 	 	 	 	 	 	 * used, so mark it as __devinitdata
     	 	 	 	 	 	 	 */
    info->flags = FBINFO_FLAG_DEFAULT;

    info->screen_base = (char __iomem *) video;
    info->screen_size = BUF_LEN;
    info->monspecs = pd_sinfo->monspecs;
    info->par = pd_sinfo->par;

// Set videomode
    mode_option = "160x160-1@6";
    retval = fb_find_mode(&info->var, info, mode_option, mode, 1, mode, 1);
    if (!retval || retval == 4){
    	// exception 5 as 4
    	kfree(mode);
    	kfree(var);
    	release_mem_region(io_data, BUF_LEN);
		release_mem_region(io_cmd, 1);
		printk(KERN_INFO "fb not find video mode %s, ret=%d\n", mode_option, retval);
    	return -EINVAL;
    }

//  Video Mode OK
    printk(KERN_INFO "set fb video mode x:%d, y:%d, bpp:%d, gray:%d, xv:%d, yv:%d\n", info->var.xres, info->var.yres, info->var.bits_per_pixel, info->var.grayscale, info->var.xres_virtual, info->var.yres_virtual);
    info->mode = mode;
    head = kmalloc(sizeof(struct list_head), GFP_KERNEL);
    head->prev=0;
    head->next=0;

// Dinamic lets fix
    am160160_fb_fix.smem_start = am160160_resources[2]->start;
    am160160_fb_fix.smem_len = BUF_LEN;
    am160160_fb_fix.mmio_start = video;
    am160160_fb_fix.mmio_len = BUF_LEN;
    memcpy(&(info->fix), &am160160_fb_fix, sizeof(struct fb_fix_screeninfo));

    info->pseudo_palette = info->par;

    printk(KERN_INFO "set i/o sram: %lX+%lX; %lX+%lX\n", am160160_fb_fix.mmio_start, am160160_fb_fix.mmio_len, am160160_fb_fix.smem_start, am160160_fb_fix.smem_len);

    cmap_len = 16;
    if (fb_alloc_cmap(&info->cmap, cmap_len, 0) < 0){
    	// exception 6 as 5 as 4
    	kfree(head);
    	kfree(mode);
    	kfree(var);
    	release_mem_region(io_data, BUF_LEN);
		release_mem_region(io_cmd, 1);
		printk(KERN_INFO "not allocated cmap\n");
    	return -ENOMEM;
    }

    if (register_framebuffer(info) < 0) {
    	// exception 7
    	fb_dealloc_cmap(&info->cmap);
    	kfree(head);
    	kfree(mode);
    	kfree(var);
    	release_mem_region(io_data, BUF_LEN);
		release_mem_region(io_cmd, 1);
		printk(KERN_INFO "fb not registered\n");
    	return -EINVAL;
    }

    platform_set_drvdata(pdev, info);

    printk(KERN_INFO "fb%d: %s frame buffer device\n", info->node, info->fix.id);

    // Remapping default console to framebuffer
//    con2fb.console = 0;
//    con2fb.framebuffer = 0;
//    event.info = &info;
//    event.data = &con2fb;
//    if (!fb_notifier_call_chain(FB_EVENT_GET_CONSOLE_MAP, &event))
//         printk(KERN_INFO "Begin Info: console %d, fb %d\n", con2fb.console, con2fb.framebuffer);
//    else printk(KERN_INFO "GET Event Error\n");
//
//    con2fb.console = 0;
//    con2fb.framebuffer = 0;
//    if (!fb_notifier_call_chain(FB_EVENT_SET_CONSOLE_MAP, &event))
//         printk(KERN_INFO "Remap OK\n");
//    else printk(KERN_INFO "Remap Error\n");
//
//    if (!fb_notifier_call_chain(FB_EVENT_GET_CONSOLE_MAP, &event))
//        printk(KERN_INFO "Begin Info: console %d, fb %d\n", con2fb.console, con2fb.framebuffer);
//    else printk(KERN_INFO "GET Event Error\n");

    return 0;
}

//    /*
//     *  Cleanup
//     */

 //static void __devexit am160160_fb_remove(struct pci_dev *dev)
static int __exit am160160_fb_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);
	printk(KERN_INFO "fb remove\n");

	if (info) {
		unregister_framebuffer(info);
		fb_dealloc_cmap(&info->cmap);
		/* ... */
		framebuffer_release(info);
		printk(KERN_INFO "fb removed. OK.\n");
	}else printk(KERN_INFO "fb don't removed. False.\n");

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

static struct platform_driver am160160_fb_driver = {
//	.probe = am160160_fb_probe,
	.remove = __exit_p(am160160_fb_remove),

#ifdef CONFIG_PM
	.suspend = am160160_fb_suspend, /* optional but recommended */
	.resume = am160160_fb_resume,   /* optional but recommended */
#endif /* CONFIG_PM */

	.driver = {
		.name = "am160160",
		.owner = THIS_MODULE,
	},
};



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
    /* Parse user speficied options (`video=am160160:') */
}
#endif /* MODULE */

static int __init am160160_fb_init(void)
{
	int ret;
	struct console *tcon;

	/*
	 *  For kernel boot options (in 'video=am160160_fb:<options>' format)
	 */

    if (device_cnt++) return -EINVAL;

#ifndef MODULE
	char *option = NULL;

	if (fb_get_options("am160160", &option))
		return -ENODEV;
	am160160_fb_setup(option);
#endif

	for(tcon=console_drivers; tcon != NULL; tcon = tcon->next){
		am160160fbcon = tcon;
		printk(KERN_INFO "find console \"%s\", ptr=%lX \n", am160160fbcon->name, (long unsigned int) tcon);
	}
	printk(KERN_INFO "write func ptr=%lX \n", (long unsigned int) am160160fbcon->write);
	printk(KERN_INFO "write func ptr=%lX \n", (long unsigned int) am160160fbcon->write);

	ret = platform_driver_probe(&am160160_fb_driver, am160160_fb_probe);

	printk(KERN_INFO "write func ptr=%lX \n", (long unsigned int) am160160fbcon->write);

	if (ret) {
		// В случае когда девайс не добавлен
		platform_driver_unregister(&am160160_fb_driver);
		return -ENODEV;
	}

	printk(KERN_INFO "device_open(%d)\n",ret);

	return ret;
}

static void __exit am160160_fb_exit(void)
{
	platform_driver_unregister(&am160160_fb_driver);
	hard->exit();
	device_cnt--;
	printk(KERN_INFO "device_closed\n");
}

/* ------------------------------------------------------------------------- */
    /*
     *  Modularization
     */

module_init(am160160_fb_init);
module_exit(am160160_fb_exit);

MODULE_AUTHOR("alex AAV");
//MODULE DESCRIPTION("driver for am160160 based by uc1698 or st7529");
MODULE_SUPPORTED_DEVICE("am160160");
MODULE_LICENSE("GPL");
